//! オブジェクトの詳細を表示するエディットコントロール

#pragma once

#include "../knowbug_core/encoding.h"

class ViewEditControl {
public:
	static auto create(HWND view_edit)->std::unique_ptr<ViewEditControl>;

	virtual ~ViewEditControl() {
	}

	virtual auto current_scroll_line() const->std::size_t = 0;

	virtual bool at_bottom() const = 0;

	virtual void scroll_to_line(std::size_t line_index) = 0;

	virtual void scroll_to_bottom() = 0;

	virtual void select_line(std::size_t line_index) = 0;

	virtual void set_text(OsStringView const& text) = 0;
};
