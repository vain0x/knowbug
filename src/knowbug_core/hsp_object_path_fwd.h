#pragma once

#include <string>
#include <vector>
#include "encoding.h"
#include "hsp_wrap_call.h"
#include "hsx.h"
#include "memory_view.h"

class HspObjects;

// HSP のオブジェクトの種類
enum class HspObjectKind {
	// ルート
	Root = 1,

	// グループ
	Group,

	// 省略 (「残りは省略されました」の部分)
	Ellipsis,

	// モジュール
	Module,

	// 静的変数
	StaticVar,

	// 配列要素
	Element,

	// 引数
	Param,

	Label,

	Str,

	Double,

	Int,

	// フレックス (モジュール変数のインスタンス)
	Flex,

	// 不明な型の値
	Unknown,

	// システム変数のリスト
	SystemVarList,

	// システム変数
	SystemVar,

	CallStack,

	CallFrame,

	// 全般
	General,

	Log,

	Script,

	// 利用不能なオブジェクト。
	// 子ノードの取得に失敗したときなどに生成される。
	Unavailable,
};

// HSP のオブジェクトを指し示すルートからの経路
class HspObjectPath
	: public std::enable_shared_from_this<HspObjectPath>
{
public:
	class Visitor;
	class Root;
	class Group;
	class Ellipsis;
	class Module;
	class StaticVar;
	class Element;
	class Param;
	class Label;
	class Str;
	class Double;
	class Int;
	class Flex;
	class Unknown;
	class SystemVarList;
	class SystemVar;
	class CallStack;
	class CallFrame;
	class General;
	class Log;
	class Script;
	class Unavailable;

	virtual	~HspObjectPath();

	HspObjectPath() {
	}

	// shared_ptr で管理されていないインスタンスを作れてしまうと shared_from_this が壊れるので、コピーやムーブを禁止する。
	HspObjectPath(HspObjectPath&& other) = delete;
	HspObjectPath(HspObjectPath const& other) = delete;
	auto operator =(HspObjectPath&& other)->HspObjectPath & = delete;
	auto operator =(HspObjectPath const& other)->HspObjectPath & = delete;

	virtual auto kind() const->HspObjectKind = 0;

	// パスとして同一かを判定する。
	// equals が検査するため、other と this の kind() が等しく、親要素も等しいと仮定してよい。
	virtual bool does_equal(HspObjectPath const& other) const = 0;

	// ハッシュテーブルのためのハッシュ値を計算する。
	virtual auto do_hash() const->std::size_t = 0;

	virtual auto parent() const->HspObjectPath const& = 0;

	virtual auto child_count(HspObjects& objects) const->std::size_t = 0;

	virtual auto child_at(std::size_t index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> = 0;

	// 表示のための子要素の個数。
	// 基本的には child_at と同じだが、表示の都合で異なる場合がある。
	// 例えば子要素が多いときはグループノードが生成される。
	virtual auto visual_child_count(HspObjects& objects) const->std::size_t;

	// 表示のための子要素を取得する。
	virtual auto visual_child_at(std::size_t child_index, HspObjects& objects) const->std::optional<std::shared_ptr<HspObjectPath const>>;

	// FIXME: 名前のないノードのときはどうする？
	virtual auto name(HspObjects& objects) const->std::u8string = 0;

	virtual bool is_array(HspObjects& objects) const {
		return false;
	}

	// パスとして同一かを判定する。
	// (クローン変数など、異なるパスが単一のオブジェクトを指すこともあるが、ここでは加味しない。)
	virtual bool equals(HspObjectPath const& other) const {
		if (this == &other) {
			return true;
		}
		return kind() == other.kind() && does_equal(other) && parent().equals(other.parent());
	}

	virtual auto hash() const->std::size_t;

	// パスが生存しているかを判定する。
	virtual auto is_alive(HspObjects& objects) const -> bool {
		if (!parent().is_alive(objects)) {
			return false;
		}

		auto sibling_count = parent().child_count(objects);
		for (auto i = std::size_t{}; i < sibling_count; i++) {
			auto sibling = parent().child_at(i, objects);
			if (equals(*sibling)) {
				return true;
			}
		}
		return false;
	}

	auto memory_view(HspObjects& objects) const->std::optional<MemoryView>;

	auto self() const->std::shared_ptr<HspObjectPath const>;

	auto as_root() const->HspObjectPath::Root const&;

	auto as_group() const->HspObjectPath::Group const&;

	auto as_ellipsis() const->HspObjectPath::Ellipsis const&;

	auto as_module() const->HspObjectPath::Module const&;

	auto as_static_var() const->HspObjectPath::StaticVar const&;

	auto as_element() const->HspObjectPath::Element const&;

	auto as_param() const->HspObjectPath::Param const&;

	auto as_label() const->HspObjectPath::Label const&;

	auto as_str() const->HspObjectPath::Str const&;

	auto as_double() const->HspObjectPath::Double const&;

	auto as_int() const->HspObjectPath::Int const&;

	auto as_flex() const->HspObjectPath::Flex const&;

	auto as_unknown() const->HspObjectPath::Unknown const&;

	auto as_system_var_list() const->HspObjectPath::SystemVarList const&;

	auto as_system_var() const->HspObjectPath::SystemVar const&;

	auto as_call_stack() const->HspObjectPath::CallStack const&;

	auto as_call_frame() const->HspObjectPath::CallFrame const&;

	auto as_general() const->HspObjectPath::General const&;

	auto as_log() const->HspObjectPath::Log const&;

	auto as_script() const->HspObjectPath::Script const&;

	auto as_unavailable() const->HspObjectPath::Unavailable const&;

	auto new_group(std::size_t offset) const->std::shared_ptr<HspObjectPath const>;

	auto new_ellipsis(std::size_t total_count) const->std::shared_ptr<HspObjectPath const>;

protected:
	auto new_module(std::size_t module_id) const->std::shared_ptr<HspObjectPath const>;

	auto new_static_var(std::size_t static_var_id) const->std::shared_ptr<HspObjectPath const>;

public:
	auto new_element(hsx::HspDimIndex const& indexes) const->std::shared_ptr<HspObjectPath const>;

protected:
	// param_index: 親要素の何番目の引数か
	auto new_param(hsx::HspParamType param_type, std::size_t param_index) const->std::shared_ptr<HspObjectPath const>;

public:
	auto new_label() const->std::shared_ptr<HspObjectPath const>;

	auto new_str() const->std::shared_ptr<HspObjectPath const>;

	auto new_double() const->std::shared_ptr<HspObjectPath const>;

	auto new_int() const->std::shared_ptr<HspObjectPath const>;

	auto new_flex() const->std::shared_ptr<HspObjectPath const>;

	auto new_unknown() const->std::shared_ptr<HspObjectPath const>;

protected:
	auto new_system_var_list() const->std::shared_ptr<HspObjectPath const>;

	auto new_system_var(hsx::HspSystemVarKind system_var_kind) const->std::shared_ptr<HspObjectPath const>;

public:
	auto new_call_stack() const->std::shared_ptr<HspObjectPath const>;

protected:
	auto new_call_frame(WcCallFrameKey const& key) const->std::shared_ptr<HspObjectPath const>;

	auto new_general() const->std::shared_ptr<HspObjectPath const>;

	auto new_log() const->std::shared_ptr<HspObjectPath const>;

	auto new_script() const->std::shared_ptr<HspObjectPath const>;

public:
	auto new_unavailable(std::u8string&& reason) const->std::shared_ptr<HspObjectPath const>;
};

static auto operator ==(HspObjectPath const& first, HspObjectPath const& second) -> bool {
	return first.equals(second);
}

static auto operator !=(HspObjectPath const& first, HspObjectPath const& second) -> bool {
	return !(first == second);
}

static auto operator ==(std::shared_ptr<HspObjectPath const> const& first, std::shared_ptr<HspObjectPath const> const& second) -> bool {
	if (!first) {
		return !second;
	}

	return first->equals(*second);
}

static auto operator !=(std::shared_ptr<HspObjectPath const> const& first, std::shared_ptr<HspObjectPath const> const& second) -> bool {
	return !(first == second);
}

namespace std {
	template<>
	struct hash<std::shared_ptr<::HspObjectPath const>> {
		auto operator()(std::shared_ptr<::HspObjectPath const> const& path) const {
			return path->hash();
		}
	};
}
