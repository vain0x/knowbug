#include "pch.h"
#include "knowbug_view_edit.h"

class ViewEditControlImpl
	: public ViewEditControl
{
	HWND view_edit_;

public:
	ViewEditControlImpl(HWND view_edit)
		: view_edit_(view_edit)
	{
	}

	auto current_scroll_line() const -> std::size_t override {
		return Edit_GetFirstVisibleLine(view_edit_);
	}

	bool at_bottom() const override {
		auto line_count = Edit_GetLineCount(view_edit_);

		// ウィンドウに30行ぐらい表示されていると仮定して、スクロールが一番下にありそうかどうか判定する。
		return line_count <= (int)current_scroll_line() + 30;
	}

	void scroll_to_line(std::size_t line_index) override {
		Edit_Scroll(view_edit_, (int)line_index, 0);
	}

	void scroll_to_bottom() override {
		auto line_count = Edit_GetLineCount(view_edit_);
		scroll_to_line(line_count);
	}

	void select_line(std::size_t line_index) override {
		auto start = Edit_LineIndex(view_edit_, (int)line_index);
		auto end = Edit_LineIndex(view_edit_, (int)line_index + 1);
		Edit_SetSel(view_edit_, start, end);
	}

	void set_text(OsStringView const& text) override {
		SetWindowText(view_edit_, text.data());
	}
};

auto ViewEditControl::create(HWND view_edit)->std::unique_ptr<ViewEditControl> {
	return std::make_unique<ViewEditControlImpl>(view_edit);
}
