#pragma once

#include <string>
#include "HspDebugApi.h"

class HspObjects;

// HSP のオブジェクトの種類
enum class HspObjectKind {
	// ルート
	Root = 1,

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

	Log,

	Script,
};

// HSP のオブジェクトを指し示すルートからの経路
class HspObjectPath
	: public std::enable_shared_from_this<HspObjectPath>
{
public:
	class Visitor;
	class Root;
	class Module;
	class StaticVar;
	class Element;
	class Param;
	class Label;
	class Str;
	class Double;
	class Int;
	class Flex;
	class Log;
	class Script;

	virtual	~HspObjectPath();

	HspObjectPath() {
	}

	// shared_ptr で管理されていないインスタンスを作れてしまうと shared_from_this が壊れるので、コピーやムーブを禁止する。
	HspObjectPath(HspObjectPath&& other) = delete;
	HspObjectPath(HspObjectPath const& other) = delete;
	auto operator =(HspObjectPath&& other) -> HspObjectPath & = delete;
	auto operator =(HspObjectPath const& other) -> HspObjectPath & = delete;

	virtual auto kind() const -> HspObjectKind = 0;

	// パスとして同一かを判定する。
	// equals が検査するため、other と this の kind() が等しく、親要素も等しいと仮定してよい。
	virtual bool does_equal(HspObjectPath const& other) const = 0;

	virtual auto parent() const -> HspObjectPath const& = 0;

	virtual auto child_count(HspObjects& objects) const -> std::size_t = 0;

	virtual auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> = 0;

	// FIXME: 名前のないノードのときはどうする？
	virtual auto name(HspObjects& objects) const -> std::string = 0;

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

	bool is_alive(HspObjects& objects) const {
		if (kind() == HspObjectKind::Root || kind() == HspObjectKind::Module) {
			return true;
		}

		auto sibling_count = parent().child_count(objects);
		for (auto i = std::size_t{}; i < sibling_count; i++) {
			auto&& sibling = parent().child_at(i, objects);
			if (equals(*sibling)) {
				return true;
			}
		}
		return false;
	}

	auto self() const -> std::shared_ptr<HspObjectPath const>;

	auto as_root() const->HspObjectPath::Root const&;

	auto as_module() const->HspObjectPath::Module const&;

	auto as_static_var() const -> HspObjectPath::StaticVar const&;

	auto as_element() const -> HspObjectPath::Element const&;

	auto as_param() const -> HspObjectPath::Param const&;

	auto as_label() const -> HspObjectPath::Label const&;

	auto as_str() const -> HspObjectPath::Str const&;

	auto as_double() const -> HspObjectPath::Double const&;

	auto as_int() const -> HspObjectPath::Int const&;

	auto as_flex() const -> HspObjectPath::Flex const&;

	auto as_log() const -> HspObjectPath::Log const&;

	auto as_script() const -> HspObjectPath::Script const&;

protected:
	auto new_module(std::size_t module_id) const->std::shared_ptr<HspObjectPath const>;

	auto new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const>;

public:
	auto new_element(HspIndexes const& indexes) const -> std::shared_ptr<HspObjectPath const>;

protected:
	// param_index: 親要素の何番目の引数か
	auto new_param(HspParamType param_type, std::size_t param_index) const -> std::shared_ptr<HspObjectPath const>;

	auto new_label() const -> std::shared_ptr<HspObjectPath const>;

	auto new_str() const -> std::shared_ptr<HspObjectPath const>;

	auto new_double() const -> std::shared_ptr<HspObjectPath const>;

	auto new_int() const -> std::shared_ptr<HspObjectPath const>;

	auto new_flex() const -> std::shared_ptr<HspObjectPath const>;

	auto new_log() const -> std::shared_ptr<HspObjectPath const>;

	auto new_script() const -> std::shared_ptr<HspObjectPath const>;
};

// -----------------------------------------------
// ルートパス
// -----------------------------------------------

class HspObjectPath::Root final
	: public HspObjectPath
{
public:
	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Root;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return kind() == other.kind();
	}

	bool equals(HspObjectPath const& other) const override {
		return kind() == other.kind();
	}

	auto parent() const -> HspObjectPath const& override;

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override;

	auto new_global_module(HspObjects& objects) const->std::shared_ptr<HspObjectPath const>;
};

// -----------------------------------------------
// モジュールパス
// -----------------------------------------------

class HspObjectPath::Module final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t module_id_;

public:
	using HspObjectPath::new_module;
	using HspObjectPath::new_static_var;

	Module(std::shared_ptr<HspObjectPath const> parent, std::size_t module_id);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Module;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return module_id() == other.as_module().module_id();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->std::string override;

	auto module_id() const -> std::size_t {
		return module_id_;
	}

	bool is_global(HspObjects& objects) const;
};

// -----------------------------------------------
// 静的変数パス
// -----------------------------------------------

class HspObjectPath::StaticVar final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t static_var_id_;

public:
	using HspObjectPath::new_element;
	using HspObjectPath::new_int;

	StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::StaticVar;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return static_var_id() == other.as_static_var().static_var_id();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override;

	bool is_array(HspObjects& objects) const override;

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto type(HspObjects& objects) const -> HspType;

	auto metadata(HspObjects& objects) const -> HspVarMetadata;
};

// -----------------------------------------------
// 配列要素
// -----------------------------------------------

class HspObjectPath::Element final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	HspIndexes indexes_;

public:
	using HspObjectPath::new_label;
	using HspObjectPath::new_str;
	using HspObjectPath::new_double;
	using HspObjectPath::new_int;
	using HspObjectPath::new_flex;

	Element(std::shared_ptr<HspObjectPath const> parent, HspIndexes indexes);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Element;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return indexes() == other.as_element().indexes();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override;

	auto indexes() const -> HspIndexes const& {
		return indexes_;
	}
};

// -----------------------------------------------
// 引数
// -----------------------------------------------

class HspObjectPath::Param final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	HspParamType param_type_;
	std::size_t param_index_;

public:
	using HspObjectPath::new_element;
	using HspObjectPath::new_str;
	using HspObjectPath::new_double;
	using HspObjectPath::new_int;

	Param(std::shared_ptr<HspObjectPath const> parent, HspParamType param_type, std::size_t param_index);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Param;
	}

	bool does_equal(HspObjectPath const& other) const override {
		auto&& o = other.as_param();
		return param_type() == o.param_type() && param_index() == o.param_index();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override;

	auto param_type() const -> HspParamType {
		return param_type_;
	}

	auto param_index() const -> std::size_t {
		return param_index_;
	}
};

// -----------------------------------------------
// ラベル
// -----------------------------------------------

class HspObjectPath::Label final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Label(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Label;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: パス自体には名前がない (ラベル名は使わない)
		return std::string{};
	}

	bool is_null(HspObjects& objects) const;

	auto static_label_name(HspObjects& objects) const -> std::optional<std::string>;

	auto static_label_id(HspObjects& objects) const -> std::optional<std::size_t>;
};

// -----------------------------------------------
// 文字列
// -----------------------------------------------

class HspObjectPath::Str final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Str(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Str;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	auto value(HspObjects& objects) const -> HspStr;
};

// -----------------------------------------------
// 浮動小数点数
// -----------------------------------------------

class HspObjectPath::Double final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Double(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Double;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	auto value(HspObjects& objects) const -> HspDouble;
};

// -----------------------------------------------
// 整数
// -----------------------------------------------

class HspObjectPath::Int final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Int(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Int;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	auto value(HspObjects& objects) const -> HspInt;
};

// -----------------------------------------------
// フレックス
// -----------------------------------------------

class HspObjectPath::Flex final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	using HspObjectPath::new_param;

	Flex(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Flex;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	bool is_nullmod(HspObjects& objects) const;

	auto module_name(HspObjects& objects) const -> char const*;
};

// -----------------------------------------------
// ログ
// -----------------------------------------------

class HspObjectPath::Log final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Log(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Log;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		return std::string{ "Log" };
	}

	auto content(HspObjects& objects) const -> std::string const&;

	void append(char const* text, HspObjects& objects) const;

	void clear(HspObjects& objects) const;
};

// -----------------------------------------------
// スクリプト
// -----------------------------------------------

class HspObjectPath::Script final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Script(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Script;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> std::string override {
		return std::string{ "script" };
	}

	auto content(HspObjects& objects) const -> std::string const&;

	// :thinking_face:
	auto current_line(HspObjects& objects) const -> std::size_t;
};

// -----------------------------------------------
// ビジター
// -----------------------------------------------

class HspObjectPath::Visitor {
	HspObjects& objects_;

public:
	explicit Visitor(HspObjects& objects);

	virtual void accept(HspObjectPath const& path);

	virtual void accept_default(HspObjectPath const& path);

	virtual void accept_parent(HspObjectPath const& path);

	virtual void accept_children(HspObjectPath const& path);

	virtual void on_root(HspObjectPath::Root const& path) {
		accept_default(path);
	}

	virtual void on_module(HspObjectPath::Module const& path) {
		accept_default(path);
	}

	virtual void on_static_var(HspObjectPath::StaticVar const& path) {
		accept_default(path);
	}

	virtual void on_element(HspObjectPath::Element const& path) {
		accept_default(path);
	}

	virtual void on_param(HspObjectPath::Param const& path) {
		accept_default(path);
	}

	virtual void on_label(HspObjectPath::Label const& path) {
		accept_default(path);
	}

	virtual void on_str(HspObjectPath::Str const& path) {
		accept_default(path);
	}

	virtual void on_double(HspObjectPath::Double const& path) {
		accept_default(path);
	}

	virtual void on_int(HspObjectPath::Int const& path) {
		accept_default(path);
	}

	virtual void on_flex(HspObjectPath::Flex const& path) {
		accept_default(path);
	}

	virtual void on_log(HspObjectPath::Log const& path) {
		accept_default(path);
	}

	virtual void on_script(HspObjectPath::Script const& path) {
		accept_default(path);
	}

protected:
	auto objects() const -> HspObjects& {
		return objects_;
	}
};
