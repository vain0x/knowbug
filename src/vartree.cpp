
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#define hwndVarTree (Dialog::getVarTreeHandle())

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif
using detail::TvObserver;
using detail::LogObserver;

static auto TreeView_MyInsertItem
	( HTREEITEM hParent, char const* name, bool sorts
	, VTNodeData* node
	) -> HTREEITEM;
static void TreeView_MyDeleteItem(HTREEITEM hItem);
static auto makeNodeName(VTNodeData const& node) -> string;
static bool isAutoOpenNode(VTNodeData const& node);

struct VTView::Impl
{
	VTView& self_;

	unordered_map<VTNodeData const*, HTREEITEM> itemFromNode_;

	//ノードの文字列のキャッシュ (停止中の間のみ有効)
	unordered_map<HTREEITEM, shared_ptr<string const>> textCache_;

	//ノードごとのビューウィンドウのキャレット位置
	unordered_map<HTREEITEM, int> viewCaret_;

	shared_ptr<TvObserver> observer_;
	shared_ptr<LogObserver> logObserver_;

	HTREEITEM hNodeDynamic_, hNodeScript_, hNodeLog_;

public:
	auto itemFromNode(VTNodeData const* p) const -> HTREEITEM;
	bool customizeTextColorIfAble(HTREEITEM hItem, LPNMTVCUSTOMDRAW pnmcd);

	int viewCaretFromNode(HTREEITEM hItem) const;
};

struct TvObserver
	: VTNodeData::Observer
{
	VTView::Impl& self;
public:
	TvObserver(VTView::Impl& self);
	void onInit(VTNodeData& node) override;
	void onTerm(VTNodeData& node) override;
};

struct LogObserver
	: VTNodeLog::LogObserver
{
	VTView::Impl& self;
public:
	LogObserver(VTView::Impl& self) : self(self) {}
	void afterAppend(char const* addition) override;
};

VTView::VTView()
	: p_(new Impl { *this })
{
	// Register observers
	p_->observer_ = std::make_shared<TvObserver>(*p_);
	VTNodeData::registerObserver(p_->observer_);

	p_->logObserver_ = std::make_shared<LogObserver>(*p_);
	VTRoot::log()->setLogObserver(p_->logObserver_);

	// Initialize tree
	VTRoot::make_shared()->updateDeep();

#ifdef with_WrapCall
	p_->hNodeDynamic_ = p_->itemFromNode(VTRoot::dynamic().get());
#endif
	p_->hNodeScript_  = p_->itemFromNode(VTRoot::script().get());
	p_->hNodeLog_     = p_->itemFromNode(VTRoot::log().get());
}

VTView::~VTView()
{
}

auto VTView::Impl::itemFromNode(VTNodeData const* p) const -> HTREEITEM
{
	auto&& iter = itemFromNode_.find(p);
	return (iter != itemFromNode_.end()) ? iter->second : nullptr;
}

TvObserver::TvObserver(VTView::Impl& self)
	: self(self)
{
	self.itemFromNode_[&VTRoot::instance()] = TVI_ROOT;
}

void TvObserver::onInit(VTNodeData& node)
{
	auto&& parent = node.parent();
	if ( ! parent ) return; // VTRoot

	auto&& hParent = self.itemFromNode(parent.get());
	assert(hParent != nullptr);

	auto&& hItem = TreeView_MyInsertItem
		( hParent
		, makeNodeName(node).c_str()
		, false
		, &node);

	assert(self.itemFromNode_[&node] == nullptr);
	self.itemFromNode_[&node] = hItem;

	self.viewCaret_.erase(hItem);

	if ( isAutoOpenNode(*parent) ) {
		TreeView_Expand(hwndVarTree, hParent, TVE_EXPAND);
	}
}

void TvObserver::onTerm(VTNodeData& node)
{
	if ( auto&& hItem = self.itemFromNode(&node) ) {
		self.itemFromNode_[&node] = nullptr;
		TreeView_MyDeleteItem(hItem);
	}
}

void LogObserver::afterAppend(char const* addition)
{
	if ( TreeView_GetSelection(hwndVarTree) == self.hNodeLog_ ) {
		Dialog::View::update();
	}
}

void VTView::update()
{
	p_->textCache_.clear();

#ifdef with_WrapCall
	VTRoot::dynamic()->updateDeep();
#endif

	Dialog::View::update();
}

auto VTView::tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData>
{
	auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
	assert(lp);
	return lp;
}

// ノードに応じて文字色を設定する
// Return true iff text color is modified.
bool VTView::Impl::customizeTextColorIfAble(HTREEITEM hItem, LPNMTVCUSTOMDRAW pnmcd)
{
	// 選択状態なら色分けしない
	if ( TreeView_GetItemState(hwndVarTree, hItem, 0) & TVIS_SELECTED ) {
		return false;
	}

	string const sItem = TreeView_GetItemString(hwndVarTree, hItem);
	char const* const name = sItem.c_str();

	auto const cont = [&pnmcd](COLORREF cref) {
		pnmcd->clrText = cref;
		return true;
	};

	auto const node = self_.tryGetNodeData(hItem);
	if ( !node ) return false;

#ifdef with_WrapCall
	if ( auto const nodeInvoke = dynamic_cast<VTNodeInvoke const*>(node) ) {
			auto const key = (nodeInvoke->callinfo().stdat->index == STRUCTDAT_INDEX_FUNC)
				? "__sttm__"
				: "__func__";
			auto const&& iter = g_config->clrTextExtra.find(key);
			if ( iter != g_config->clrTextExtra.end() ) {
				return cont(iter->second);
			}
	} else
#endif //defined(with_WrapCall)
	{
		vartype_t const vtype = node->vartype();
		if ( 0 < vtype && vtype < HSPVAR_FLAG_USERDEF ) {
			return cont(g_config->clrText[vtype]);

		} else if ( vtype >= HSPVAR_FLAG_USERDEF ) {
			auto const&& iter = g_config->clrTextExtra.find(hpiutil::varproc(vtype)->vartype_name);
			if ( iter != g_config->clrTextExtra.end() ) {
				return cont(iter->second);
			}
		}
	}
	return false;
}

// 変数ツリーの NM_CUSTOMDRAW を処理する
LRESULT VTView::customDraw( LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;

	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		auto const hItem = reinterpret_cast<HTREEITEM>(pnmcd->nmcd.dwItemSpec);
		bool const modified = p_->customizeTextColorIfAble(hItem, pnmcd);
		if ( modified ) {
			return CDRF_NEWFONT;
		}
	}
	return 0;
}

// ノードに対応する文字列を得る
auto VTView::getItemVarText(HTREEITEM hItem) const -> std::shared_ptr<string const>
{
	struct GetText
		: public VTNodeData::Visitor
	{
		CVarinfoText varinf;
		shared_ptr<string const> result;

		void fModule(VTNodeModule const& node) override
		{
			varinf.addModuleOverview(node.name().c_str(), node);
		}
		void fVar(VTNodeVar const& node) override
		{
			varinf.addVar(node.pval(), node.name().c_str());
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			varinf.addSysvarsOverview();
		}
		void fLog(VTNodeLog const& node) override
		{
			result = shared_ptr_from_rawptr(&node.str());
		}
		void fScript(VTNodeScript const& node) override
		{
			if ( auto&& p = node.fetchScriptAll(g_dbginfo->curFileName()) ) {
				result = shared_ptr_from_rawptr(p);
			} else {
				result = std::make_shared<string>(g_dbginfo->getCurInfString());
			}
		}
		void fGeneral(VTNodeGeneral const&) override
		{
			varinf.addGeneralOverview();
		}
		void fSysvar(VTNodeSysvar const& node) override
		{
			varinf.addSysvar(node.id());
		}
#ifdef with_WrapCall
		void fDynamic(VTNodeDynamic const&) override
		{
			varinf.addCallsOverview(VTRoot::dynamic()->lastIndependedResult().get());
		}
		void fInvoke(VTNodeInvoke const& node) override
		{
			varinf.addCall(node.callinfo());
		}
		void fResult(VTNodeResult const& node) override
		{
			assert(usesResultNodes());
			varinf.addResult(node);
		}
#endif
		auto apply(VTNodeData const& node) -> shared_ptr<string const>
		{
			node.acceptVisitor(*this);
			return (result)
				? result
				: std::make_shared<string>(varinf.getStringMove());
		}
	};

	auto&& get = [this, &hItem] () {
		if ( auto&& node = tryGetNodeData(hItem) ) {
			return GetText {}.apply(*node);
		} else {
			return std::make_shared<string const>("(not_available)");
		}
	};

	auto&& stringPtr =
		(g_config->cachesVardataString && hItem != p_->hNodeLog_)
		? map_find_or_insert(p_->textCache_, hItem, std::move(get))
		: get();
	assert(stringPtr);
	return stringPtr;
}

void VTView::saveCurrentViewCaret(int vcaret)
{
	if ( HTREEITEM const hItem = TreeView_GetSelection(hwndVarTree) ) {
		p_->viewCaret_[hItem] = vcaret;
	}
}

int VTView::Impl::viewCaretFromNode(HTREEITEM hItem) const
{
	auto&& iter = viewCaret_.find(hItem);
	return (iter != viewCaret_.end() ? iter->second : 0);
}

void VTView::selectNode(VTNodeData const& node)
{
	if ( auto&& hItem = p_->itemFromNode(&node) ) {
		TreeView_SelectItem(hwndVarTree, hItem);
	}
}

void VTView::updateViewWindow()
{
	HTREEITEM const hItem = TreeView_GetSelection(hwndVarTree);
	if ( hItem ) {
		static HTREEITEM stt_prevSelection = nullptr;
		if ( hItem == stt_prevSelection ) {
			Dialog::View::saveCurrentCaret();
		} else {
			stt_prevSelection = hItem;
		}

		auto&& varinfoText = getItemVarText(hItem);
		Dialog::View::setText(varinfoText->c_str());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == p_->hNodeScript_ ) {
			int const iLine = g_dbginfo->curLine();
			Dialog::View::scroll(std::max(0, iLine - 3), 0);
			Dialog::View::selectLine(iLine);

		//+log ノードの自動スクロール
		} else if ( hItem == p_->hNodeLog_
			&& g_config->scrollsLogAutomatically
			) {
			Dialog::View::scrollBottom();

		} else {
			Dialog::View::scroll(p_->viewCaretFromNode(hItem), 0);
		}
	}
}

static HTREEITEM TreeView_MyInsertItem
	( HTREEITEM hParent
	, char const* name
	, bool sorts
	, VTNodeData* node)
{
	TVINSERTSTRUCT tvis {};
	tvis.hParent = hParent;
	tvis.hInsertAfter = (sorts ? TVI_SORT : TVI_LAST);
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.lParam  = (LPARAM)node;
	tvis.item.pszText = const_cast<char*>(name);

	return TreeView_InsertItem(hwndVarTree, &tvis);
}

static void TreeView_MyDeleteItem(HTREEITEM hItem)
{
	TreeView_EscapeFocus(hwndVarTree, hItem);
	TreeView_DeleteItem(hwndVarTree, hItem);
}

// ノードにつけるべき名前
auto makeNodeName(VTNodeData const& node) -> string
{
	struct matcher : VTNodeData::Visitor
	{
		string result;
		string apply(VTNodeData const& node)
		{
			result = node.name(); // default
			node.acceptVisitor(*this);
			return std::move(result);
		}

		void fInvoke(VTNodeInvoke const& node) override { result = "\'" + node.name(); }
		void fResult(VTNodeResult const& node) override { result = "\"" + node.name(); }
	};

	return matcher {}.apply(node);
}

// 自動的に開くべきノードか？
static bool isAutoOpenNode(VTNodeData const& node)
{
	struct matcher : VTNodeData::Visitor
	{
		bool result;
		bool apply(VTNodeData const& node)
		{
			result = true; // default
			node.acceptVisitor(*this);
			return std::move(result);
		}

		void fModule(VTNodeModule const& node) override
		{
			result = (node.name() == "@");
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			result = false;
		}
	};

	return matcher {}.apply(node);
}
