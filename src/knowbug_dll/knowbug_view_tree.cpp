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
	, public HspObjectTreeObserver
{
	HspObjects& objects_;
	HspObjectTree& object_tree_;

	HWND tree_view_;
	std::unordered_map<HTREEITEM, std::size_t> node_ids_;
	std::unordered_map<std::size_t, HTREEITEM> node_tv_items_;

	// 自動で展開されるべきノードのリスト。
	// NOTE: ノードはツリービューに追加した直後には展開できず、さらに子ノードを持たないと展開できない模様。
	//       そのため、ノードが追加された段階ではこの配列にノードを追加することで「自動展開の予約」を行い、
	//       更新後のイベント (did_update) で展開可能か検査し、初めて可能になったときに展開する。
	std::vector<HTREEITEM> auto_expand_items_;

	// NOTE: 削除されたノードに関してエディットコントロールが持っている情報を削除するため、
	//       削除されたノードをここに記録して、次の更新時にまとめて削除依頼を出す。
	std::vector<HTREEITEM> tv_items_for_view_edit_control_to_forget_;

public:
	VarTreeViewControlImpl(HspObjects& objects, HspObjectTree& object_tree, HWND tree_view)
		: objects_(objects)
		, object_tree_(object_tree)
		, tree_view_(tree_view)
		, node_ids_()
		, node_tv_items_()
		, auto_expand_items_()
		, tv_items_for_view_edit_control_to_forget_()
	{
		node_ids_.emplace(TVI_ROOT, object_tree_.root_id());
		node_tv_items_.emplace(object_tree_.root_id(), TVI_ROOT);

		object_tree_.focus_root(*this);
	}

	void did_initialize() override {
		select_global();
		do_auto_expand_all();
	}

	// オブジェクトツリーが更新されたときに呼ばれる。
	void did_create(std::size_t node_id, HspObjectTreeInsertMode mode) override {
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

		auto tv_item = do_insert_item(tv_parent, to_os(name), insert_mode_to_sibling(mode));
		node_ids_.emplace(tv_item, node_id);
		node_tv_items_.emplace(node_id, tv_item);

		auto_expand(*path, tv_item);
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
		tv_items_for_view_edit_control_to_forget_.push_back(tv_item);
	}

	void update_view_window(ViewEditControl& view_edit_control) override {
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
		{
			// 遅延されていたデータの削除を実行する。
			{
				auto& tv_items = tv_items_for_view_edit_control_to_forget_;
				for (auto tv_item : tv_items) {
					view_edit_control.forget(tv_item);
				}
				tv_items.clear();
			}

			auto text = object_path_to_text(path, objects_);
			auto cursor_policy = object_path_to_cursor_policy(path, objects_);

			view_edit_control.update(tv_item, text, cursor_policy);
		}
	}

	void did_update() override {
		do_auto_expand_all();
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

	void select_global() {
		auto global_path = objects_.root_path().new_global_module(objects_);
		auto node_id = object_tree_.focus_by_path(*global_path, *this);
		auto tv_item_opt = node_to_tv_item(node_id);
		if (tv_item_opt) {
			do_select_item(*tv_item_opt);
		}
	}

	void auto_expand(HspObjectPath const& path, HTREEITEM tv_item) {
		if (object_path_is_auto_expand(path, objects_)) {
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

auto VarTreeViewControl::create(HspObjects& objects, HspObjectTree& object_tree, HWND tree_view) -> std::unique_ptr<VarTreeViewControl> {
	return std::make_unique<VarTreeViewControlImpl>(objects, object_tree, tree_view);
}
