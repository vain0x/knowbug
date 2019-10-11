//! 変数ツリービュー関連

#include "pch.h"
#include <unordered_map>
#include <unordered_set>
#include "../knowbug_core/HspObjectPath.h"
#include "../knowbug_core/HspObjects.h"
#include "../knowbug_core/HspObjectTree.h"
#include "../knowbug_core/HspObjectWriter.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/string_writer.h"
#include "knowbug_view_tree.h"
#include "win_gui.h"

#undef min

static auto object_path_to_text(HspObjectPath const& path, HspObjects& objects) -> OsString {
	// FIXME: 共通化
	auto writer = CStrWriter{};
	HspObjectWriter{ objects, writer }.write_table_form(path);
	auto text = as_utf8(writer.finish());

	return to_os(std::move(text));
}

// ツリービューのノードを自動で開くか否か
static auto object_path_is_auto_expand(HspObjectPath const& path, HspObjects& objects) -> bool {
	return path.kind() == HspObjectKind::CallStack
		|| (path.kind() == HspObjectKind::Module && path.as_module().is_global(objects));
}

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

class VarTreeViewControlImpl
	: public VarTreeViewControl
	, public HspObjectTreeObserver
{
	HspObjects& objects_;
	HspObjectTree& object_tree_;

	HWND tree_view_;
	std::unordered_map<HTREEITEM, std::size_t> node_ids_;
	std::unordered_map<std::size_t, HTREEITEM> node_tv_items_;

	std::unordered_set<HTREEITEM> auto_expand_items_;

	ScrollPreserver scroll_preserver_;

public:
	VarTreeViewControlImpl(HspObjects& objects, HspObjectTree& object_tree, HWND tree_view)
		: objects_(objects)
		, object_tree_(object_tree)
		, tree_view_(tree_view)
		, node_ids_()
		, node_tv_items_()
		, auto_expand_items_()
		, scroll_preserver_()
	{
		node_ids_.emplace(TVI_ROOT, object_tree_.root_id());
		node_tv_items_.emplace(object_tree_.root_id(), TVI_ROOT);

		object_tree_.focus_root(*this);
	}

	void did_initialize() override {
		do_auto_expand();
	}

	// オブジェクトツリーが更新されたときに呼ばれる。
	void did_create(std::size_t node_id) override {
		auto&& path_opt = object_tree_.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		auto&& name = path->name(objects_);

		auto&& parent_id_opt = object_tree_.parent(node_id);
		auto tv_parent = parent_id_opt && node_tv_items_.count(*parent_id_opt)
			? node_tv_items_.at(*parent_id_opt)
			: TVI_ROOT;

		auto tv_item = do_insert_item(tv_parent, as_view(to_os(name)));
		node_ids_.emplace(tv_item, node_id);
		node_tv_items_.emplace(node_id, tv_item);

		if (object_path_is_auto_expand(*path, objects_)) {
			auto_expand_items_.emplace(tv_item);
		}
	}

	// オブジェクトツリーのノードが破棄される前に呼ばれる。
	void will_destroy(std::size_t node_id) override {
		auto&& path_opt = object_tree_.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		assert(node_tv_items_.count(node_id) && u8"存在しないノードが削除されようとしています");
		auto tv_item = node_tv_items_.at(node_id);

		do_delete_item(tv_item);
		node_ids_.erase(tv_item);
		node_tv_items_.erase(node_id);
	}

	void update_view_window(AbstractViewBox& view_box) override {
		// コールスタックを自動で更新する。
		object_tree_.focus_by_path(*objects_.root_path().new_call_stack(), *this);

		auto&& selected_node_id_opt = this->selected_node_id();
		if (!selected_node_id_opt) {
			return;
		}

		// フォーカスを当てる。
		auto focused_node_id = object_tree_.focus(*selected_node_id_opt, *this);

		// フォーカスの当たった要素のパスとハンドル。
		auto&& path_opt = object_tree_.path(focused_node_id);
		auto&& tv_item_opt = node_to_tv_item(focused_node_id);

		if (!path_opt || !tv_item_opt) {
			return;
		}
		auto&& path = **path_opt;
		auto&& tv_item = *tv_item_opt;

		// ビューウィンドウを更新する。
		// スクロール位置を保存して、文字列を交換して、スクロール位置を適切に戻す。
		auto text = object_path_to_text(path, objects_);

		scroll_preserver_.will_activate(tv_item, view_box);

		view_box.set_text(as_view(text));

		scroll_preserver_.did_activate(tv_item, path, objects_, view_box);

		auto_expand(path, tv_item);
	}

	auto log_is_selected() const -> bool override {
		if (auto&& node_id_opt = selected_node_id()) {
			if (auto&& path_opt = object_tree_.path(*node_id_opt)) {
				if ((*path_opt)->kind() == HspObjectKind::Log) {
					return true;
				}
			}
		}
		return false;
	}

	auto item_to_path(HTREEITEM tv_item) const -> std::optional<std::shared_ptr<HspObjectPath const>> override {
		if (auto&& node_id_opt = selected_node_id()) {
			if (auto&& path_opt = object_tree_.path(*node_id_opt)) {
				return path_opt;
			}
		}
		return std::nullopt;
	}

private:
	auto node_id(HTREEITEM tv_item) const -> std::optional<std::size_t> {
		auto&& iter = node_ids_.find(tv_item);
		if (iter == node_ids_.end()) {
			return std::nullopt;
		}

		return std::make_optional(iter->second);
	}

	auto node_to_tv_item(std::size_t node_id) const -> std::optional<HTREEITEM> {
		auto&& iter = node_tv_items_.find(node_id);
		if (iter == node_tv_items_.end()) {
			return std::nullopt;
		}

		return std::make_optional(iter->second);
	}

	auto selected_node_id() const -> std::optional<std::size_t> {
		return node_id(selected_tv_item());
	}

	auto selected_tv_item() const -> HTREEITEM {
		return TreeView_GetSelection(tree_view_);
	}

	void auto_expand(HspObjectPath const& path, HTREEITEM tv_item) {
		if (object_path_is_auto_expand(path, objects_)) {
			auto_expand_items_.emplace(tv_item);
		}
	}

	void do_auto_expand() {
		for (auto tv_item : auto_expand_items_) {
			do_expand_item(tv_item);
		}
		auto_expand_items_.clear();
	}

	auto do_insert_item(HTREEITEM hParent, OsStringView const& name) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		tvis.hParent = hParent;
		tvis.hInsertAfter = TVI_LAST; // FIXME: 引数で受け取る
		tvis.item.mask = TVIF_TEXT;
		tvis.item.pszText = const_cast<LPTSTR>(name.data());
		res = TreeView_InsertItem(tree_view_, &tvis);
		return res;
	}

	void do_delete_item(HTREEITEM hItem) {
		TreeView_EscapeFocus(tree_view_, hItem);
		TreeView_DeleteItem(tree_view_, hItem);
	}

	void do_expand_item(HTREEITEM hParent) {
		TreeView_Expand(tree_view_, hParent, TVE_EXPAND);
	}

	void do_select_item(HTREEITEM hItem) {
		TreeView_SelectItem(tree_view_, hItem);
	}
};

auto VarTreeViewControl::create(HspObjects& objects, HspObjectTree& object_tree, HWND tree_view) -> std::unique_ptr<VarTreeViewControl> {
	return std::make_unique<VarTreeViewControlImpl>(objects, object_tree, tree_view);
}
