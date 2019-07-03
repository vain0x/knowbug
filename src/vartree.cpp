
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"
#include "Logger.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspObjectTree.h"
#include "HspStaticVars.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#include "module/supio/supio.h"

#undef min

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

	auto insert_item(HTREEITEM hParent, OsStringView const& name, VTNodeData* node) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST; // FIXME: 引数で受け取る (コールスタックでは先頭への挿入が起こる)
		tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
		tvis.item.lParam = (LPARAM)node;
		tvis.item.pszText = const_cast<LPTSTR>(name.data());
		res = TreeView_InsertItem(hwndVarTree, &tvis);
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

class HspObjectTreeObserverImpl;

static auto makeNodeName(VTNodeData const& node) -> string;
static bool isAutoOpenNode(VTNodeData const& node);

// ビューのスクロール位置を計算するもの
class ScrollPreserver {
	HTREEITEM active_item_;
	std::unordered_map<HTREEITEM, std::size_t> scroll_lines_;
	std::unordered_set<HTREEITEM> at_bottom_;

public:
	ScrollPreserver()
		: active_item_(TVI_ROOT)
		, scroll_lines_()
		, at_bottom_()
	{
	}

	void will_activate(HTREEITEM item, AbstractViewBox& view_box) {
		auto old_scroll_line = view_box.current_scroll_line();

		if (active_item_ == item) {
			save_scroll_line(item, old_scroll_line);
		} else {
			active_item_ = item;
		}

		if (view_box.at_bottom()) {
			at_bottom_.emplace(item);
		} else {
			at_bottom_.erase(item);
		}
	}

	void did_activate(HTREEITEM item, HspObjectPath const& path, HspObjects& objects, AbstractViewBox& view_box) {
		switch (path.kind()) {
		case HspObjectKind::Log:
			{
				if (at_bottom_.count(item)) {
					view_box.scroll_to_bottom();
					return;
				}

				scroll_to_default(item, view_box);
				return;
			}
		case HspObjectKind::Script:
			{
				auto current_line = path.as_script().current_line(objects);
				auto scroll_line = current_line - std::min(current_line, std::size_t{ 3 });

				view_box.scroll_to_line(scroll_line);
				view_box.select_line(current_line);
				return;
			}
		default:
			scroll_to_default(item, view_box);
			return;
		}
	}

private:
	void scroll_to_default(HTREEITEM item, AbstractViewBox& view_box) {
		view_box.scroll_to_line(get_scroll_line(item));
	}

	void save_scroll_line(HTREEITEM item, std::size_t scroll_line) {
		scroll_lines_[item] = scroll_line;
	}

	auto get_scroll_line(HTREEITEM item) -> std::size_t {
		auto&& iter = scroll_lines_.find(item);
		if (iter == scroll_lines_.end()) {
			return 0;
		}

		return iter->second;
	}
};

struct VTView::Impl
{
	VTView& self_;

	unordered_map<VTNodeData const*, HTREEITEM> itemFromNode_;

	//ノードごとのビューウィンドウのキャレット位置
	unordered_map<HTREEITEM, int> viewCaret_;

	shared_ptr<TvObserver> observer_;
	shared_ptr<LogObserver> logObserver_;

	HTREEITEM hNodeDynamic_, hNodeScript_, hNodeLog_;

	std::shared_ptr<HspObjectTreeObserverImpl> tree_observer_;
	ScrollPreserver scroll_preserver_;

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

class HspObjectTreeObserverImpl
	: public HspObjectTreeObserver
{
	hpiutil::DInfo const& debug_segment_;
	HspObjects& objects_;
	HspObjectTree& object_tree_;
	HspStaticVars& static_vars_;

	HWND tv_handle_;
	std::unordered_map<HTREEITEM, std::size_t> node_ids_;
	std::unordered_map<std::size_t, HTREEITEM> node_handles_;

public:
	HspObjectTreeObserverImpl(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspObjectTree& object_tree, HspStaticVars& static_vars, HWND tv_handle)
		: debug_segment_(debug_segment)
		, objects_(objects)
		, object_tree_(object_tree)
		, static_vars_(static_vars)
		, tv_handle_(tv_handle)
		, node_ids_()
		, node_handles_()
	{
		node_ids_.emplace(TVI_ROOT, object_tree_.root_id());
		node_handles_.emplace(object_tree_.root_id(), TVI_ROOT);
	}

	auto tv() -> VarTreeView {
		return VarTreeView{ tv_handle_ };
	}

	auto node_id(HTREEITEM node_handle) const -> std::optional<std::size_t> {
		auto&& iter = node_ids_.find(node_handle);
		if (iter == node_ids_.end()) {
			return std::nullopt;
		}

		return std::make_optional(iter->second);
	}

	auto item_handle(std::size_t node_id) const -> std::optional<HTREEITEM> {
		auto&& iter = node_handles_.find(node_id);
		if (iter == node_handles_.end()) {
			return std::nullopt;
		}

		return std::make_optional(iter->second);
	}

	void log(std::string&& text) {
		auto&& text_os_str = to_os(as_hsp(std::move(text)));
		Knowbug::get_logger()->append_line(as_view(text_os_str));
	}

	virtual void did_create(std::size_t node_id) {
		auto&& path_opt = object_tree_.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		auto&& name = path->name(objects_);
		log(strf("create '%s' (%d)", as_native(as_view(name)).data(), node_id));

		{
			auto node_name = to_os(name);

			auto&& parent_id_opt = object_tree_.parent(node_id);

			auto parent_handle = parent_id_opt && node_handles_.count(*parent_id_opt)
				? node_handles_.at(*parent_id_opt)
				: TVI_ROOT;

			auto item_handle = tv().insert_item(parent_handle, as_view(node_name), nullptr);

			node_ids_.emplace(item_handle, node_id);
			node_handles_.emplace(node_id, item_handle);

			tv().expand_item(item_handle);
		}
	}

	virtual void will_destroy(std::size_t node_id) {
		auto&& path_opt = object_tree_.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		auto&& name = path->name(objects_);
		log(strf("destroy '%s' (%d)", as_native(as_view(name)).data(), node_id));

		{
			assert(node_handles_.count(node_id) && u8"存在しないノードが削除されようとしています");
			auto item_handle = node_handles_.at(node_id);

			tv().delete_item(item_handle);

			node_ids_.erase(item_handle);
			node_handles_.erase(node_id);
		}
	}

	auto selected_node_id() -> std::optional<std::size_t> {
		auto item = tv().selected_item();

		auto&& iter = node_ids_.find(item);
		if (iter == node_ids_.end()) {
			return std::nullopt;
		}
		return std::make_optional(iter->second);
	}
};

VTView::VTView(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars, HspObjectTree& object_tree, HWND tv_handle)
	: debug_segment_(debug_segment)
	, objects_(objects)
	, static_vars_(static_vars)
	, object_tree_(object_tree)
	, p_(new Impl { *this })
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

	// 新APIの部分
	p_->tree_observer_ = std::make_shared<HspObjectTreeObserverImpl>(debug_segment_, objects_, object_tree_, static_vars_, tv_handle);
	object_tree_.subscribe(std::weak_ptr<HspObjectTreeObserver>{ p_->tree_observer_ });
	object_tree_.focus_root();
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

	auto name = to_os(as_hsp(makeNodeName(node)));
	auto const hItem = tv.insert_item(hParent, as_view(name), &node);

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
	class GetText
		: public VTNodeData::Visitor
	{
		CVarinfoText varinf;
		unique_ptr<OsString> result;

	public:
		GetText(CVarinfoText&& varinf)
			: varinf(std::move(varinf))
		{
		}

		void fModule(VTNodeModule const& node) override
		{
			varinf.add(*node.path());
		}
		void fVar(VTNodeVar const& node) override
		{
			auto const& path = node.path();
			if (path) {
				varinf.add(*path);
				return;
			}

			varinf.addVar(node.pval(), node.name().c_str());
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			varinf.addSysvarsOverview();
		}
		void fLog(VTNodeLog const& node) override
		{
			result = std::make_unique<OsString>(to_owned(node.content())); // FIXME: 無駄なコピー
		}
		void fScript(VTNodeScript const& node) override
		{
			if ( auto p = node.fetchScriptAll(g_dbginfo->curPos().fileRefName()) ) {
				result = std::move(p);
			} else {
				auto&& cur_inf = g_dbginfo->getCurInfString();
				result = std::make_unique<OsString>(to_os(as_hsp(cur_inf.data())));
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
				: std::make_unique<OsString>(to_os(as_utf8(varinf.getString().data())));
		}
	};

	if ( auto node = tryGetNodeData(hItem) ) {
		return GetText{ CVarinfoText{ debug_segment_, objects_, static_vars_ } }.apply(*node);
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

void VTView::updateViewWindow(AbstractViewBox& view_box)
{
	auto tv = VarTreeView{ hwndVarTree };

	auto const hItem = tv.selected_item();
	if ( hItem ) {
		// 新API
		{
			// テキストを更新する
			if (auto&& node_id_opt = p_->tree_observer_->node_id(hItem)) {
				// フォーカスを当てる。
				auto node_id = object_tree_.focus(*node_id_opt);

				// フォーカスの当たった要素のパスとハンドル。
				auto&& path_opt = object_tree_.path(node_id);
				auto&& item_handle_opt = p_->tree_observer_->item_handle(node_id);

				if (path_opt && item_handle_opt) {
					auto&& path = *path_opt;
					auto&& item_handle = *item_handle_opt;

					auto varinf = CVarinfoText{ debug_segment_, objects_, static_vars_ };
					varinf.add(*path);
					auto text = to_os(as_utf8(varinf.getString().data()));

					// ビューウィンドウに反映する。
					// スクロール位置を保存して、文字列を交換して、スクロール位置を適切に戻す。
					p_->scroll_preserver_.will_activate(item_handle, view_box);

					Dialog::View::setText(as_view(text));

					p_->scroll_preserver_.did_activate(item_handle, *path, objects_, view_box);
					return;
				}
			}
		}

		static auto stt_prevSelection = HTREEITEM { nullptr };
		if ( hItem == stt_prevSelection ) {
			Dialog::View::saveCurrentCaret();
		} else {
			stt_prevSelection = hItem;
		}

		auto varinfoText = getItemVarText(hItem);
		Dialog::View::setText(as_view(*varinfoText));

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

void VTView::did_log_change() {
	auto log_is_selected = false;
	if (auto&& node_id_opt = p_->tree_observer_->selected_node_id()) {
		if (auto&& path_opt = object_tree_.path(*node_id_opt)) {
			if ((*path_opt)->kind() == HspObjectKind::Log) {
				log_is_selected = true;
			}
		}
	}

	if (log_is_selected) {
		update();
	}
}

auto VTView::item_to_path(HTREEITEM tree_item) -> std::optional<std::shared_ptr<HspObjectPath const>> {
	if (auto&& node_id_opt = p_->tree_observer_->selected_node_id()) {
		if (auto&& path_opt = object_tree_.path(*node_id_opt)) {
			return path_opt;
		}
	}
	return std::nullopt;
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
