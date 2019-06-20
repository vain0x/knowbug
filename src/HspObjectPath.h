#pragma once

#include <string>

class HspObjects;

// HSP のオブジェクトの種類
enum class HspObjectKind {
	// ルート
	Root = 1,

	// モジュール
	Module,

	// 静的変数
	StaticVar,
};

// HSP のオブジェクトを指し示すルートからの経路
class HspObjectPath
	: public std::enable_shared_from_this<HspObjectPath>
{
public:
	class Root;
	class Module;
	class StaticVar;

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

	virtual auto parent() const -> std::shared_ptr<HspObjectPath const> const& = 0;

	virtual auto child_count(HspObjects& objects) const -> std::size_t = 0;

	virtual auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> = 0;

	// FIXME: 名前のないノードもある
	virtual auto name(HspObjects& objects) const -> std::string = 0;

	virtual bool is_array(HspObjects& objects) const {
		return false;
	}

	auto self() const -> std::shared_ptr<HspObjectPath const>;

	auto as_module() const->HspObjectPath::Module const&;

	auto new_module(std::size_t module_id) const->std::shared_ptr<HspObjectPath const>;

	auto as_static_var() const -> HspObjectPath::StaticVar const&;

	auto new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const>;
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
};
