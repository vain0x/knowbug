#pragma once

#include <string>

class HspObjects;

// HSP のオブジェクトの種類
enum class HspObjectKind {
	// ルート
	Root,

	// 静的変数
	StaticVar,
};

// HSP のオブジェクトを指し示すルートからの経路
class HspObjectPath
	: public std::enable_shared_from_this<HspObjectPath>
{
public:
	class Root;
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

	virtual auto parent() const -> std::shared_ptr<HspObjectPath const> const& = 0;

	virtual auto kind() const -> HspObjectKind = 0;

	virtual auto name(HspObjects& objects) const -> std::string = 0;

	virtual bool is_array(HspObjects& objects) const {
		return false;
	}

	auto self() const -> std::shared_ptr<HspObjectPath const>;

	auto as_static_var() const -> HspObjectPath::StaticVar const&;

	auto new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const>;
};

// -----------------------------------------------
// ルートノード
// -----------------------------------------------

class HspObjectPath::Root final
	: public HspObjectPath
{
public:
	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override;

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Root;
	}

	auto name(HspObjects& objects) const -> std::string override;
};

// -----------------------------------------------
// 静的変数
// -----------------------------------------------

class HspObjectPath::StaticVar final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t static_var_id_;

public:
	StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id);

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto parent() const -> std::shared_ptr<HspObjectPath const> const& override {
		return parent_;
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::StaticVar;
	}

	auto name(HspObjects& objects) const -> std::string override;

	bool is_array(HspObjects& objects) const override;
};
