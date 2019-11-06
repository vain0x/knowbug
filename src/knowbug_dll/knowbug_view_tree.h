//! 変数ツリービュー関連

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"
#include "knowbug_view_edit.h"

class HspObjectPath;
class HspObjects;
class HspObjectTree;

class VarTreeViewControl {
public:
	static auto create(HspObjectTree& object_tree, HWND tree_view) -> std::unique_ptr<VarTreeViewControl>;

	virtual ~VarTreeViewControl() {
	}

	virtual auto selected_node_id_opt() const->std::optional<std::size_t> = 0;

	virtual void did_initialize(HspObjects& objects, HspObjectTree& object_tree) = 0;

	virtual void did_update() = 0;

	virtual void update_view_window(HspObjects& objects, HspObjectTree& object_tree, ViewEditControl& view_edit_control) = 0;

	virtual void object_node_did_create(std::size_t node_id, HspObjectTreeInsertMode mode, HspObjects& objects, HspObjectTree& object_tree) = 0;

	virtual void object_node_will_destroy(std::size_t node_id, HspObjectTree& object_tree, ViewEditControl& view_edit_control) = 0;

	// :thinking_face:
	virtual auto item_to_path(HTREEITEM tree_item, HspObjectTree& object_tree) const -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;
};
