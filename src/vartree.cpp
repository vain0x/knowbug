
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

namespace VarTree
{

static HTREEITEM g_hNodeScript, g_hNodeLog;

 //ノードの文字列のキャッシュ (停止中の間のみ有効)
static std::map<HTREEITEM, shared_ptr<string const>> g_textCache;

//ノードごとのビューウィンドウのキャレット位置
static std::map<HTREEITEM, int> g_viewCaret;

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;

static HTREEITEM g_hNodeDynamic;
#endif

static auto TreeView_MyInsertItem
	( HTREEITEM hParent, char const* name, bool sorts
	, shared_ptr<VTNodeData> node) -> HTREEITEM;
static void TreeView_MyDeleteItem(HTREEITEM hItem);

static auto makeNodeName(VTNodeData const& node) -> string;

class TvRepr
{
public:
	TvRepr()
	{
		observer_ = std::make_shared<TvObserver>(*this);
		VTNodeData::registerObserver(observer_);

		VTRoot::log()->setLogObserver(std::make_shared<LogObserver>());
	}
	~TvRepr()
	{
		VTNodeData::unregisterObserver(observer_);
	}

	auto itemFromNode(VTNodeData const* p) -> HTREEITEM
	{
		auto&& iter = itemFromNode_.find(p);
		return (iter != itemFromNode_.end()) ? iter->second : nullptr;
	}

private:
	struct TvObserver;
	friend struct TvObserver;

	struct TvObserver
		: VTNodeData::Observer
	{
		TvRepr& self;

	public:
		TvObserver(TvRepr& self)
			: self(self)
		{
			self.itemFromNode_[&VTRoot::instance()] = TVI_ROOT;
		}

		void onInit(VTNodeData& node) override
		{
			if ( !node.parent() ) return; // VTRoot

			auto&& hParent = self.itemFromNode(node.parent().get());
			auto&& hItem = TreeView_MyInsertItem
				( hParent
				, makeNodeName(node).c_str()
				, false
				, node.shared_from_this());

			assert(self.itemFromNode_[&node] == nullptr);
			self.itemFromNode_[&node] = hItem;

			g_viewCaret.erase(hItem);

			// TODO: @, +dynamic, 呼び出しノードは自動的に開く
		}
		void onTerm(VTNodeData& node) override
		{
			if ( auto&& hItem = self.itemFromNode(&node) ) {
				self.itemFromNode_[&node] = nullptr;
				TreeView_MyDeleteItem(hItem);
			}
		}
	};

	struct LogObserver : VTNodeLog::LogObserver
	{
		void afterAppend(char const* addition) override
		{
			if ( TreeView_GetSelection(hwndVarTree) == g_hNodeLog ) {
				Dialog::View::update();
			}
		}
	};

private:
	std::map<VTNodeData const*, HTREEITEM> itemFromNode_;
	shared_ptr<TvObserver> observer_;
};

static std::unique_ptr<TvRepr> g_tv;

void init()
{
	g_tv.reset(new TvRepr());

	VTRoot::make_shared()->updateDeep();

#ifdef with_WrapCall
	g_hNodeDynamic = g_tv->itemFromNode(VTRoot::dynamic().get());
#endif
	g_hNodeScript  = g_tv->itemFromNode(VTRoot::script().get());
	g_hNodeLog     = g_tv->itemFromNode(VTRoot::log().get());
	
	//@, +dynamic は開いておく
#ifdef with_WrapCall
	TreeView_Expand(hwndVarTree, g_hNodeDynamic, TVE_EXPAND);
#endif
	HTREEITEM const hRoot = TreeView_GetRoot(hwndVarTree);
	assert(TreeView_GetItemString(hwndVarTree, hRoot) == VTNodeModule::Global::Name);
	TreeView_Expand(hwndVarTree, hRoot, TVE_EXPAND);

	TreeView_EnsureVisible(hwndVarTree, hRoot); //トップまでスクロール
	TreeView_SelectItem(hwndVarTree, hRoot);
}

void term()
{
	g_tv.reset();
}

void update()
{
	g_textCache.clear();

#ifdef with_WrapCall
	VTRoot::dynamic()->updateDeep();
#endif

	Dialog::View::update();
}

static HTREEITEM TreeView_MyInsertItem
	( HTREEITEM hParent
	, char const* name
	, bool sorts
	, shared_ptr<VTNodeData> node)
{
	TVINSERTSTRUCT tvis {};
	tvis.hParent = hParent;
	tvis.hInsertAfter = (sorts ? TVI_SORT : TVI_LAST);
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.lParam  = (LPARAM)node.get();
	tvis.item.pszText = const_cast<char*>(name);

	return TreeView_InsertItem(hwndVarTree, &tvis);
}

auto tryGetNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>
{
	auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
	assert(lp);
	try {
		return lp->shared_from_this();
	} catch ( std::bad_weak_ptr const& ) {
		return nullptr;
	}
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

// ノードに応じて文字色を設定する
// Return true iff text color is modified.
static bool customizeTextColorIfAble(HTREEITEM hItem, LPNMTVCUSTOMDRAW pnmcd)
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

	auto const node = tryGetNodeData(hItem);
	if ( !node ) return false;

#ifdef with_WrapCall
	if ( auto const nodeInvoke = std::dynamic_pointer_cast<VTNodeInvoke const>(node) ) {
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
LRESULT customDraw( LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;

	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		auto const hItem = reinterpret_cast<HTREEITEM>(pnmcd->nmcd.dwItemSpec);
		bool const modified = customizeTextColorIfAble(hItem, pnmcd);
		if ( modified ) {
			return CDRF_NEWFONT;
		}
	}
	return 0;
}

// ノードに対応する文字列を得る
std::shared_ptr<string const> getItemVarText( HTREEITEM hItem )
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

	auto&& get = [&hItem] () {
		if ( auto&& node = tryGetNodeData(hItem) ) {
			return GetText {}.apply(*node);
		} else {
			return std::make_shared<string const>("(not_available)");
		}
	};

	auto&& stringPtr =
		(g_config->cachesVardataString && hItem != g_hNodeLog)
		? map_find_or_insert(g_textCache, hItem, std::move(get))
		: get();
	assert(stringPtr);
	return stringPtr;
}

void saveCurrentViewCaret(int vcaret)
{
	if ( HTREEITEM const hItem = TreeView_GetSelection(hwndVarTree) ) {
		g_viewCaret[hItem] = vcaret;
	}
}

int viewCaretFromNode(HTREEITEM hItem)
{
	auto&& iter = g_viewCaret.find(hItem);
	return (iter != g_viewCaret.end() ? iter->second : 0);
}

void selectNode(VTNodeData const& node)
{
	if ( auto&& hItem = g_tv->itemFromNode(&node) ) {
		TreeView_SelectItem(hwndVarTree, hItem);
	}
}

} // namespace VarTree

void Dialog::View::update()
{
	HTREEITEM const hItem = TreeView_GetSelection(hwndVarTree);
	if ( hItem ) {
		static HTREEITEM stt_prevSelection = nullptr;
		if ( hItem == stt_prevSelection ) {
			Dialog::View::saveCurrentCaret();
		} else {
			stt_prevSelection = hItem;
		}

		std::shared_ptr<string const> varinfoText = VarTree::getItemVarText(hItem);
		Dialog::View::setText(varinfoText->c_str());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == VarTree::g_hNodeScript ) {
			int const iLine = g_dbginfo->curLine();
			Dialog::View::scroll(std::max(0, iLine - 3), 0);
			Dialog::View::selectLine(iLine);

			//+log ノードの自動スクロール
		} else if ( hItem == VarTree::g_hNodeLog
			&& g_config->scrollsLogAutomatically
			) {
			Dialog::View::scrollBottom();

		} else {
			Dialog::View::scroll(VarTree::viewCaretFromNode(hItem), 0);
		}
	}
}
