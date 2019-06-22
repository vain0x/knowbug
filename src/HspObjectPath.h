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

	Str,

	Int,

	// フレックス (構造体やモジュール変数の型)
	Flex,
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
	class Str;
	class Int;
	class Flex;

	static auto get_root() -> HspObjectPath::Root const&;

	virtual	~HspObjectPath();

	HspObjectPath()
	{
	}

	HspObjectPath(HspObjectPath&& other) = delete;

	HspObjectPath(HspObjectPath const& other) = delete;

	auto operator =(HspObjectPath&& other) -> HspObjectPath & = delete;

	auto operator =(HspObjectPath const& other) -> HspObjectPath & = delete;

	virtual auto kind() const -> HspObjectKind = 0;

	// FIXME: HspObjectPath const& を返す方がいいかも
	virtual auto parent() const -> std::shared_ptr<HspObjectPath const> const& = 0;

	virtual auto child_count(HspObjects& objects) const -> std::size_t = 0;

	virtual auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> = 0;

	// FIXME: 名前のないノードのときはどうする？
	virtual auto name(HspObjects& objects) const -> std::string = 0;

	virtual bool is_array(HspObjects& objects) const {
		return false;
	}

	auto self() const -> std::shared_ptr<HspObjectPath const>;

	auto as_root() const->HspObjectPath::Root const&;

	auto as_module() const->HspObjectPath::Module const&;

	auto as_static_var() const -> HspObjectPath::StaticVar const&;

	auto as_element() const -> HspObjectPath::Element const&;

	auto as_str() const -> HspObjectPath::Str const&;

	auto as_int() const -> HspObjectPath::Int const&;

	auto as_flex() const -> HspObjectPath::Flex const&;

protected:
	auto new_module(std::size_t module_id) const->std::shared_ptr<HspObjectPath const>;

	auto new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const>;

	auto new_element(HspIndexes const& indexes) const -> std::shared_ptr<HspObjectPath const>;

	auto new_str() const -> std::shared_ptr<HspObjectPath const>;

	auto new_int() const -> std::shared_ptr<HspObjectPath const>;

	auto new_flex() const -> std::shared_ptr<HspObjectPath const>;
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

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override;

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

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
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

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
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
	using HspObjectPath::new_int;

	Element(std::shared_ptr<HspObjectPath const> parent, HspIndexes indexes);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Element;
	}

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override;

	auto indexes() const -> HspIndexes const& {
		return indexes_;
	}

	auto type(HspObjects& objects) const -> HspType;
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

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		throw new std::exception{ "no children" };
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	auto value(HspObjects& objects) const -> HspStr;
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

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		throw new std::exception{ "no children" };
	}

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	auto value(HspObjects& objects) const -> HspInt;
};

// -----------------------------------------------
// 構造体
// -----------------------------------------------

class HspObjectPath::Flex final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Flex(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Flex;
	}

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::string override {
		// FIXME: 名前自体がない
		return std::string{};
	}

	bool is_nullmod(HspObjects& objects) const;
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

	virtual void on_str(HspObjectPath::Str const& path) {
		accept_default(path);
	}

	virtual void on_int(HspObjectPath::Int const& path) {
		accept_default(path);
	}

	virtual void on_flex(HspObjectPath::Flex const& path) {
		accept_default(path);
	}

protected:
	auto objects() const -> HspObjects& {
		return objects_;
	}
};
