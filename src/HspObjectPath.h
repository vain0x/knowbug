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
class HspObjectPath {
public:
	class Root;
	class StaticVar;

	static auto get_root() -> HspObjectPath::Root const&;

	// インターフェイス:

	virtual	~HspObjectPath();

	virtual auto parent() const -> std::shared_ptr<HspObjectPath> const& = 0;

	virtual auto kind() const -> HspObjectKind = 0;

	virtual auto name(HspObjects& objects) const -> std::string = 0;

	virtual bool is_array(HspObjects& objects) const {
		return false;
	}

	// ダウンキャスト:

	auto as_static_var() const -> HspObjectPath::StaticVar const& {
		if (kind() != HspObjectKind::StaticVar) {
			throw new std::bad_cast{};
		}
		return *(HspObjectPath::StaticVar const*)this;
	}
};

// -----------------------------------------------
// ルートノード
// -----------------------------------------------

class HspObjectPath::Root final
	: public HspObjectPath
{
public:
	auto parent() const -> std::shared_ptr<HspObjectPath> const& override;

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
	std::shared_ptr<HspObjectPath> parent_;
	std::size_t static_var_id_;

public:
	StaticVar(std::shared_ptr<HspObjectPath> parent, std::size_t static_var_id);

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto parent() const -> std::shared_ptr<HspObjectPath> const& override {
		return parent_;
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::StaticVar;
	}

	auto name(HspObjects& objects) const -> std::string override;

	bool is_array(HspObjects& objects) const override;
};
