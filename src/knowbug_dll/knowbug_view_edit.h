//! オブジェクトの詳細を表示するエディットコントロール

#pragma once

#include "../knowbug_core/encoding.h"

// エディットコントロールのカーソルがどこにあるのが自然かを規定する。
class CursorPolicy {
public:
	enum class Kind {
		// カーソルが一番上にあるのが自然
		Top,

		// カーソルが末尾にあるのが自然
		Bottom,

		// 特定の行を指すのが自然
		SelectLine,
	};

private:
	Kind kind_;
	std::size_t line_index_;

	CursorPolicy(Kind kind, std::size_t line_index)
		: kind_(kind)
		, line_index_(line_index)
	{
	}

public:
	static auto new_top() -> CursorPolicy {
		return CursorPolicy{ Kind::Top, std::size_t{} };
	}

	static auto new_bottom() -> CursorPolicy {
		return CursorPolicy{ Kind::Bottom, std::size_t{} };
	}

	static auto new_select_line(std::size_t line_index) -> CursorPolicy {
		return CursorPolicy{ Kind::SelectLine, line_index };
	}

	auto kind() const->Kind {
		return kind_;
	}

	auto line_index() const->std::size_t {
		assert(kind() == Kind::SelectLine);
		return line_index_;
	}
};

class ViewEditControl {
public:
	// エンティティの識別子
	// エンティティごとにカーソルの状態を記録するので、エンティティを区別するための識別子がいる。
	// NOTE: いまのところエンティティとはツリービューのノードのこと。
	using EntityId = void const*;

	static auto create(HWND view_edit)->std::unique_ptr<ViewEditControl>;

	virtual ~ViewEditControl() {
	}

	// あるエンティティに関する文字列を表示する。
	virtual void update(EntityId entity_id, OsStringView const& text, CursorPolicy cursor_policy) = 0;

	// あるエンティティに関する情報を消去する。
	virtual void forget(EntityId entity_id) = 0;
};
