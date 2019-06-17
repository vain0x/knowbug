
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"
#include "Logger.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#include "module/supio/supio.h"

// 変数ツリービューに対する操作のラッパー
class VarTreeView {
	HWND hwndVarTree;

public:
	VarTreeView(HWND tree_view_handle)
		: hwndVarTree(tree_view_handle)
	{
	}

	auto selected_item() const -> HTREEITEM {
		return TreeView_GetSelection(hwndVarTree);
	}

	auto insert_item(HTREEITEM hParent, char const* name, VTNodeData* node) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		HSPAPICHAR* hactmp1;
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST;
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvis.item.lParam = (LPARAM)node;
		tvis.item.pszText = chartoapichar(const_cast<char*>(name), &hactmp1);
		res = TreeView_InsertItem(hwndVarTree, &tvis);
		freehac(&hactmp1);
		return res;
	}

	void delete_item(HTREEITEM hItem) {
		TreeView_EscapeFocus(hwndVarTree, hItem);
		TreeView_DeleteItem(hwndVarTree, hItem);
	}

	void expand_item(HTREEITEM hParent) {
		TreeView_Expand(hwndVarTree, hParent, TVE_EXPAND);
	}

	void select_item(HTREEITEM hItem) {
		TreeView_SelectItem(hwndVarTree, hItem);
	}

	auto tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData> {
		auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
		assert(lp);
		return lp;
	}
};

#define hwndVarTree (Dialog::getVarTreeHandle())

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif
using detail::TvObserver;
using detail::VarTreeLogObserver;

static auto makeNodeName(VTNodeData const& node) -> string;
static bool isAutoOpenNode(VTNodeData const& node);

struct VTView::Impl
{
	VTView& self_;

	unordered_map<VTNodeData const*, HTREEITEM> itemFromNode_;

	//ノードごとのビューウィンドウのキャレット位置
	unordered_map<HTREEITEM, int> viewCaret_;

	shared_ptr<TvObserver> observer_;
	shared_ptr<LogObserver> logObserver_;

	HTREEITEM hNodeDynamic_, hNodeScript_, hNodeLog_;

public:
	auto itemFromNode(VTNodeData const* p) const -> HTREEITEM;

	auto viewCaretFromNode(HTREEITEM hItem) const -> int;
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

struct VarTreeLogObserver
	: LogObserver
{
	VTView::Impl& self;
public:
	VarTreeLogObserver(VTView::Impl& self) : self(self) {}
	void did_change() override;
};

VTView::VTView()
	: p_(new Impl { *this })
{
	// Register observers
	p_->observer_ = std::make_shared<TvObserver>(*p_);
	VTNodeData::registerObserver(p_->observer_);

	p_->logObserver_ = std::make_shared<VarTreeLogObserver>(*p_);
	VTRoot::log().setLogObserver(p_->logObserver_);

	// Initialize tree
	VTRoot::instance().updateDeep();

#ifdef with_WrapCall
	p_->hNodeDynamic_ = p_->itemFromNode(&VTRoot::dynamic());
#endif
	p_->hNodeScript_  = p_->itemFromNode(&VTRoot::script());
	p_->hNodeLog_     = p_->itemFromNode(&VTRoot::log());
}

VTView::~VTView()
{
}

auto VTView::Impl::itemFromNode(VTNodeData const* p) const -> HTREEITEM
{
	auto const iter = itemFromNode_.find(p);
	return (iter != itemFromNode_.end()) ? iter->second : nullptr;
}

TvObserver::TvObserver(VTView::Impl& self)
	: self(self)
{
	self.itemFromNode_[&VTRoot::instance()] = TVI_ROOT;
}

void TvObserver::onInit(VTNodeData& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	auto const parent = node.parent();
	if ( ! parent ) return; // VTRoot

	auto const hParent = self.itemFromNode(parent);
	assert(hParent != nullptr);

	auto const hItem = tv.insert_item(hParent, makeNodeName(node).c_str(), &node);

	assert(self.itemFromNode_[&node] == nullptr);
	self.itemFromNode_[&node] = hItem;

	self.viewCaret_.erase(hItem);

	if ( isAutoOpenNode(*parent) ) {
		tv.expand_item(hParent);
	}
}

void TvObserver::onTerm(VTNodeData& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = self.itemFromNode(&node) ) {
		self.itemFromNode_[&node] = nullptr;
		tv.delete_item(hItem);
	}
}

void VarTreeLogObserver::did_change()
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( tv.selected_item() == self.hNodeLog_ ) {
		Dialog::View::update();
	}
}

void VTView::update()
{
#ifdef with_WrapCall
	VTRoot::dynamic().updateDeep();
#endif

	Dialog::View::update();
}

auto VTView::tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData> {
	return VarTreeView{ hwndVarTree }.tryGetNodeData(hItem);
}

// ノードに対応する文字列を得る
auto VTView::getItemVarText(HTREEITEM hItem) const -> std::unique_ptr<OsString>
{
	struct GetText
		: public VTNodeData::Visitor
	{
		CVarinfoText varinf;
		unique_ptr<OsString> result;

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
			result = std::make_unique<OsString>(node.content().to_owned()); // FIXME: 無駄なコピー
		}
		void fScript(VTNodeScript const& node) override
		{
			if ( auto p = node.fetchScriptAll(g_dbginfo->curPos().fileRefName()) ) {
				result = std::move(p);
			} else {
				result = std::make_unique<OsString>(HspStringView{ g_dbginfo->getCurInfString().data() }.to_os_string());
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
			varinf.addCallsOverview();
		}
		void fInvoke(VTNodeInvoke const& node) override
		{
			varinf.addCall(node.callinfo());
		}
#endif
		auto apply(VTNodeData const& node) -> unique_ptr<OsString>
		{
			node.acceptVisitor(*this);
			return result
				? std::move(result)
				: std::make_unique<OsString>(HspStringView{ varinf.getString().data() }.to_os_string());
		}
	};

	if ( auto node = tryGetNodeData(hItem) ) {
		return GetText {}.apply(*node);
	} else {
		return std::make_unique<OsString>(TEXT("(not_available)"));
	}
}

void VTView::saveCurrentViewCaret(int vcaret)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = tv.selected_item() ) {
		p_->viewCaret_[hItem] = vcaret;
	}
}

auto VTView::Impl::viewCaretFromNode(HTREEITEM hItem) const -> int
{
	auto iter = viewCaret_.find(hItem);
	return (iter != viewCaret_.end() ? iter->second : 0);
}

void VTView::selectNode(VTNodeData const& node)
{
	auto tv = VarTreeView{ hwndVarTree };

	if ( auto const hItem = p_->itemFromNode(&node) ) {
		tv.select_item(hItem);
	}
}

void VTView::updateViewWindow()
{
	auto tv = VarTreeView{ hwndVarTree };

	auto const hItem = tv.selected_item();
	if ( hItem ) {
		static auto stt_prevSelection = HTREEITEM { nullptr };
		if ( hItem == stt_prevSelection ) {
			Dialog::View::saveCurrentCaret();
		} else {
			stt_prevSelection = hItem;
		}

		auto varinfoText = getItemVarText(hItem);
		Dialog::View::setText(varinfoText->as_ref());

		//+script ノードなら現在の実行位置を選択
		if ( hItem == p_->hNodeScript_ ) {
			auto const iLine = g_dbginfo->curPos().line();
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

// ノードにつけるべき名前
auto makeNodeName(VTNodeData const& node) -> string
{
	struct matcher : VTNodeData::Visitor
	{
		auto apply(VTNodeData const& node) -> string
		{
			result = node.name(); // default
			node.acceptVisitor(*this);
			return std::move(result);
		}

		void fInvoke(VTNodeInvoke const& node) override { result = "\'" + node.name(); }
	private:
		string result;
	};

	return matcher {}.apply(node);
}

// 自動的に開くべきノードか？
static bool isAutoOpenNode(VTNodeData const& node)
{
	struct matcher : VTNodeData::Visitor
	{
		bool apply(VTNodeData const& node)
		{
			result = true; // default
			node.acceptVisitor(*this);
			return result;
		}

		void fModule(VTNodeModule const& node) override
		{
			result = (node.name() == "@");
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			result = false;
		}
	private:
		bool result;
	};

	return matcher {}.apply(node);
}
