#include "pch.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include "../knowbug_core/platform.h"
#include "knowbug_view_edit.h"

// スクリプトの現在行の周囲何行を表示するか。
static auto constexpr SCRIPT_LINE_CONTEXT = std::size_t{ 3 };

// 0 未満にならない引き算
static auto monus(std::size_t first, std::size_t second) -> std::size_t {
	return first - std::min(first, second);
}

class ViewEditControlImpl
	: public ViewEditControl
{
	class EntityData {
	public:
		// 一番上に表示されている行番号
		std::size_t scroll_line_;

		// 一番下までスクロールされているか否か
		bool at_bottom_;

		EntityData()
			: scroll_line_(0)
			, at_bottom_(true)
		{
		}
	};

	HWND view_edit_;

	EntityId active_entity_id_;
	std::unordered_map<EntityId, EntityData> entities_;

public:
	ViewEditControlImpl(HWND view_edit)
		: view_edit_(view_edit)
		, active_entity_id_()
		, entities_()
	{
	}

	void update(EntityId entity_id, OsStringView const& text, CursorPolicy cursor_policy) override {
		save_state(active_entity_id_);

		do_update_text(text);
		load_state(entity_id, cursor_policy);
		active_entity_id_ = entity_id;
	}

	void forget(EntityId entity_id) override {
		entities_.erase(entity_id);
	}

private:
	// エンティティIDが未登録なら登録し、関連するデータへの参照を取得する。
	auto touch_entity(EntityId entity_id) -> EntityData& {
		return entities_.try_emplace(entity_id, EntityData{}).first->second;
	}

	void save_state(EntityId entity_id) {
		auto& entity_data = touch_entity(entity_id);
		entity_data.scroll_line_ = current_scroll_line();
		entity_data.at_bottom_ = at_bottom();
	}

	void load_state(EntityId entity_id, CursorPolicy cursor_policy) {
		auto entity_data = touch_entity(entity_id);

		switch (cursor_policy.kind()) {
		case CursorPolicy::Kind::Top:
			scroll_to_line(entity_data.scroll_line_);
			break;

		case CursorPolicy::Kind::Bottom:
			if (entity_data.at_bottom_) {
				scroll_to_bottom();
			} else {
				scroll_to_line(entity_data.scroll_line_);
			}
			break;

		case CursorPolicy::Kind::SelectLine: {
			scroll_to_line(monus(cursor_policy.line_index(), SCRIPT_LINE_CONTEXT));
			select_line(cursor_policy.line_index());
			break;
		}
		default:
			assert(false && u8"Unknown CursorPolicy::Kind");
		}
	}

	auto current_scroll_line() const -> std::size_t {
		return Edit_GetFirstVisibleLine(view_edit_);
	}

	// 一番下までスクロールされているか否か
	bool at_bottom() const {
		auto line_count = Edit_GetLineCount(view_edit_);

		// ウィンドウに30行ぐらい表示されていると仮定して、スクロールが一番下にありそうかどうか判定する。
		return line_count <= (int)current_scroll_line() + 30;
	}

	void scroll_to_line(std::size_t line_index) {
		Edit_Scroll(view_edit_, (int)line_index, 0);
	}

	void scroll_to_bottom() {
		auto line_count = Edit_GetLineCount(view_edit_);
		scroll_to_line(line_count);
	}

	void select_line(std::size_t line_index) {
		auto start = Edit_LineIndex(view_edit_, (int)line_index);
		auto end = Edit_LineIndex(view_edit_, (int)line_index + 1);
		Edit_SetSel(view_edit_, start, end);
	}

	void do_update_text(OsStringView const& text) {
		SetWindowText(view_edit_, text.data());
	}
};

auto ViewEditControl::create(HWND view_edit)->std::unique_ptr<ViewEditControl> {
	return std::make_unique<ViewEditControlImpl>(view_edit);
}
