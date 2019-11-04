//! 変数ツリービュー関連

#include "pch.h"
#include <unordered_map>
#include <unordered_set>
#include "../knowbug_core/hsp_object_path.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsp_object_tree.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/string_writer.h"
#include "knowbug_view_tree.h"
#include "win_gui.h"

#undef min

static auto object_path_to_text(HspObjectPath const& path, HspObjects& objects) -> OsString {
	// FIXME: 共通化
	auto writer = StringWriter{};
	HspObjectWriter{ objects, writer }.write_table_form(path);
	auto text = writer.finish();

	return to_os(std::move(text));
}

static auto object_path_to_cursor_policy(HspObjectPath const& path, HspObjects& objects) -> CursorPolicy {
	switch (path.kind()) {
	case HspObjectKind::Script:
		return CursorPolicy::new_select_line(path.as_script().current_line(objects));

	case HspObjectKind::Log:
		return CursorPolicy::new_bottom();

	default:
		return CursorPolicy::new_top();
	}
}

// ツリービューのノードを自動で開くか否か
static auto object_path_is_auto_expand(HspObjectPath const& path, HspObjects& objects) -> bool {
	return path.kind() == HspObjectKind::CallStack
		|| (path.kind() == HspObjectKind::Module && path.as_module().is_global(objects));
}

static auto insert_mode_to_sibling(HspObjectTreeInsertMode mode) {
	switch (mode) {
	case HspObjectTreeInsertMode::Front:
		return TVI_FIRST;

	case HspObjectTreeInsertMode::Back:
		return TVI_LAST;

	default:
		assert(false);
		throw std::exception{};
	}
}

class VarTreeViewControlImpl
	: public VarTreeViewControl
{
	HWND tree_view_;
	std::unordered_map<HTREEITEM, std::size_t> node_ids_;
	std::unordered_map<std::size_t, HTREEITEM> node_tv_items_;

	// 自動で展開されるべきノードのリスト。
	// NOTE: ノードはツリービューに追加した直後には展開できず、さらに子ノードを持たないと展開できない模様。
	//       そのため、ノードが追加された段階ではこの配列にノードを追加することで「自動展開の予約」を行い、
	//       更新後のイベント (did_update) で展開可能か検査し、初めて可能になったときに展開する。
	std::vector<HTREEITEM> auto_expand_items_;

public:
	VarTreeViewControlImpl(HspObjectTree& object_tree, HWND tree_view)
		: tree_view_(tree_view)
		, node_ids_()
		, node_tv_items_()
		, auto_expand_items_()
	{
		node_ids_.emplace(TVI_ROOT, object_tree.root_id());
		node_tv_items_.emplace(object_tree.root_id(), TVI_ROOT);
	}

	void did_initialize(HspObjects& objects, HspObjectTree& object_tree, HspObjectTreeObserver& observer) override {
		object_tree.focus_root(observer);
		select_global(objects, object_tree, observer);
		do_auto_expand_all();
	}

	// オブジェクトツリーが更新されたときに呼ばれる。
	void object_node_did_create(std::size_t node_id, HspObjectTreeInsertMode mode, HspObjects& objects, HspObjectTree& object_tree) override {
		auto&& path_opt = object_tree.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		auto&& name = path->name(objects);

		auto&& parent_id_opt = object_tree.parent(node_id);
		auto tv_parent = parent_id_opt && node_tv_items_.count(*parent_id_opt)
			? node_tv_items_.at(*parent_id_opt)
			: TVI_ROOT;

		auto tv_item = do_insert_item(tv_parent, to_os(name), insert_mode_to_sibling(mode));
		node_ids_.emplace(tv_item, node_id);
		node_tv_items_.emplace(node_id, tv_item);

		auto_expand(*path, tv_item, objects);
	}

	// オブジェクトツリーのノードが破棄される前に呼ばれる。
	void object_node_will_destroy(std::size_t node_id, HspObjectTree& object_tree, ViewEditControl& view_edit_control) override {
		auto&& path_opt = object_tree.path(node_id);
		if (!path_opt) {
			return;
		}
		auto&& path = *path_opt;

		assert(node_tv_items_.count(node_id) && u8"存在しないノードが削除されようとしています");
		auto tv_item = node_tv_items_.at(node_id);

		do_delete_item(tv_item);
		node_ids_.erase(tv_item);
		node_tv_items_.erase(node_id);

		view_edit_control.forget(tv_item);
	}

	void update_view_window(HspObjects& objects, HspObjectTree& object_tree, HspObjectTreeObserver& observer, ViewEditControl& view_edit_control) override {
		// コールスタックを自動で更新する。
		object_tree.focus_by_path(*objects.root_path().new_call_stack(), observer);

		auto&& selected_node_id_opt = this->selected_node_id();
		if (!selected_node_id_opt) {
			return;
		}

		// フォーカスを当てる。
		auto focused_node_id = object_tree.focus(*selected_node_id_opt, observer);

		// フォーカスの当たった要素のパスとハンドル。
		auto&& path_opt = object_tree.path(focused_node_id);
		auto&& tv_item_opt = node_to_tv_item(focused_node_id);

		if (!path_opt || !tv_item_opt) {
			return;
		}
		auto&& path = **path_opt;
		auto&& tv_item = *tv_item_opt;

		// ビューウィンドウを更新する。
		{
			auto text = object_path_to_text(path, objects);
			auto cursor_policy = object_path_to_cursor_policy(path, objects);

			view_edit_control.update(tv_item, text, cursor_policy);
		}
	}

	void did_update() override {
		do_auto_expand_all();
	}

	auto log_is_selected(HspObjectTree& object_tree) const -> bool override {
		if (auto&& node_id_opt = selected_node_id()) {
			if (auto&& path_opt = object_tree.path(*node_id_opt)) {
				if ((*path_opt)->kind() == HspObjectKind::Log) {
					return true;
				}
			}
		}
		return false;
	}

	auto item_to_path(HTREEITEM tv_item, HspObjectTree& object_tree) const -> std::optional<std::shared_ptr<HspObjectPath const>> override {
		if (auto&& node_id_opt = selected_node_id()) {
			if (auto&& path_opt = object_tree.path(*node_id_opt)) {
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

	void select_global(HspObjects& objects, HspObjectTree& object_tree, HspObjectTreeObserver& observer) {
		auto global_path = objects.root_path().new_global_module(objects);
		auto node_id = object_tree.focus_by_path(*global_path, observer);
		auto tv_item_opt = node_to_tv_item(node_id);
		if (tv_item_opt) {
			do_select_item(*tv_item_opt);
		}
	}

	void auto_expand(HspObjectPath const& path, HTREEITEM tv_item, HspObjects& objects) {
		if (object_path_is_auto_expand(path, objects)) {
			auto_expand_items_.push_back(tv_item);
		}
	}

	void do_auto_expand_all() {
		for (auto i = auto_expand_items_.size(); i >= 1;) {
			i--;

			auto tv_item = auto_expand_items_[i];
			if (!has_child(tv_item)) {
				continue;
			}

			do_expand_item(tv_item);

			// この要素を除去する。
			std::swap(auto_expand_items_[i], auto_expand_items_.back());
			auto_expand_items_.pop_back();
		}
	}

	auto has_child(HTREEITEM tv_item) const -> bool {
		auto first_child = TreeView_GetChild(tree_view_, tv_item);
		return first_child != nullptr;
	}

	auto do_insert_item(HTREEITEM hParent, OsStringView const& name, HTREEITEM sibling) -> HTREEITEM {
		auto tvis = TVINSERTSTRUCT{};
		HTREEITEM res;
		tvis.hParent = hParent;
		tvis.hInsertAfter = sibling;
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

auto VarTreeViewControl::create(HspObjectTree& object_tree, HWND tree_view) -> std::unique_ptr<VarTreeViewControl> {
	return std::make_unique<VarTreeViewControlImpl>(object_tree, tree_view);
}
