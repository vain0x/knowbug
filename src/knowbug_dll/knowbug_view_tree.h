//! 変数ツリービュー関連

#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"

class HspObjectPath;
class HspObjects;
class HspObjectTree;

class AbstractViewBox {
public:
	virtual ~AbstractViewBox() {
	}

	virtual auto current_scroll_line() const -> std::size_t = 0;

	virtual bool at_bottom() const = 0;

	virtual void scroll_to_line(std::size_t line_index) = 0;

	virtual void scroll_to_bottom() = 0;

	virtual void select_line(std::size_t line_index) = 0;

	virtual void set_text(OsStringView const& text) = 0;
};

class VarTreeViewControl {
public:
	static auto create(HspObjects& objects, HspObjectTree& object_tree, HWND tree_view) -> std::unique_ptr<VarTreeViewControl>;

	virtual ~VarTreeViewControl() {
	}

	virtual void update_view_window(AbstractViewBox& view_box) = 0;

	virtual auto log_is_selected() const -> bool = 0;

	// :thinking_face:
	virtual auto item_to_path(HTREEITEM tree_item) const -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;
};
