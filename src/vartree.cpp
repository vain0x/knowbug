
#include <Windows.h>
#include <CommCtrl.h>

#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspObjectTree.h"
#include "HspObjectWriter.h"
#include "HspStaticVars.h"

#include "vartree.h"

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

	auto insert_item(HTREEITEM hParent, OsStringView const& name) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST; // FIXME: 引数で受け取る (コールスタックでは先頭への挿入が起こる)
		tvis.item.mask = TVIF_TEXT;
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
};

#define hwndVarTree (Dialog::getVarTreeHandle())

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif

class HspObjectTreeObserverImpl;

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

	std::shared_ptr<HspObjectTreeObserverImpl> tree_observer_;
	ScrollPreserver scroll_preserver_;
};

class HspObjectTreeObserverImpl
	: public HspObjectTreeObserver
{
	hpiutil::DInfo const& debug_segment_;
	HspObjects& objects_;
	HspObjectTree& object_tree_;

	HWND tv_handle_;
	std::unordered_map<HTREEITEM, std::size_t> node_ids_;
	std::unordered_map<std::size_t, HTREEITEM> node_handles_;

public:
	HspObjectTreeObserverImpl(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspObjectTree& object_tree, HWND tv_handle)
		: debug_segment_(debug_segment)
		, objects_(objects)
		, object_tree_(object_tree)
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

	virtual void did_create(std::size_t node_id) {
		auto&& path_opt = object_tree_.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		auto&& name = path->name(objects_);

		{
			auto node_name = to_os(name);

			auto&& parent_id_opt = object_tree_.parent(node_id);

			auto parent_handle = parent_id_opt && node_handles_.count(*parent_id_opt)
				? node_handles_.at(*parent_id_opt)
				: TVI_ROOT;

			auto item_handle = tv().insert_item(parent_handle, as_view(node_name));

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

VTView::VTView(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspObjectTree& object_tree, HWND tv_handle)
	: debug_segment_(debug_segment)
	, objects_(objects)
	, object_tree_(object_tree)
	, p_(new Impl { *this })
{
	p_->tree_observer_ = std::make_shared<HspObjectTreeObserverImpl>(debug_segment_, objects_, object_tree_, tv_handle);
	object_tree_.subscribe(std::weak_ptr<HspObjectTreeObserver>{ p_->tree_observer_ });
	object_tree_.focus_root();
}

VTView::~VTView()
{
}

void VTView::update()
{
	Dialog::View::update();
}

void VTView::updateViewWindow(AbstractViewBox& view_box)
{
	auto tv = VarTreeView{ hwndVarTree };
	auto hItem = tv.selected_item();

	if (!hItem) {
		return;
	}

	auto&& node_id_opt = p_->tree_observer_->node_id(hItem);
	if (!node_id_opt) {
		return;
	}

	// フォーカスを当てる。
	auto node_id = object_tree_.focus(*node_id_opt);

	// フォーカスの当たった要素のパスとハンドル。
	auto&& path_opt = object_tree_.path(node_id);
	auto&& item_handle_opt = p_->tree_observer_->item_handle(node_id);

	if (!path_opt || !item_handle_opt) {
		return;
	}
	auto&& path = *path_opt;
	auto&& item_handle = *item_handle_opt;

	// 文字列を生成する。
	static auto const MAX_TEXT_LENGTH = std::size_t{ 0x8000 };
	auto buffer = std::make_shared<CStrBuf>();
	buffer->limit(MAX_TEXT_LENGTH);
	auto writer = CStrWriter{ buffer };
	HspObjectWriter{ objects_, writer }.write_table_form(*path);
	auto text = to_os(as_utf8(buffer->getMove()));

	// ビューウィンドウに反映する。
	// スクロール位置を保存して、文字列を交換して、スクロール位置を適切に戻す。
	p_->scroll_preserver_.will_activate(item_handle, view_box);

	view_box.set_text(as_view(text));

	p_->scroll_preserver_.did_activate(item_handle, *path, objects_, view_box);
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
