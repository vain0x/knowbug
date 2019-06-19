#pragma once

#include <string>

class HspObjects;

enum class HspObjectKind {
	StaticVar,
};

// HSP のオブジェクトを指し示すルートからの経路
class HspObjectPath {
public:
	virtual	~HspObjectPath();

	class StaticVar;

	virtual auto parent() const -> std::shared_ptr<HspObjectPath> const& = 0;

	virtual auto kind() const -> HspObjectKind = 0;

	virtual auto name(HspObjects& objects) const -> std::string = 0;

	virtual bool is_array(HspObjects& objects) const = 0;

	auto as_static_var() const -> HspObjectPath::StaticVar const& {
		if (kind() != HspObjectKind::StaticVar) {
			throw new std::bad_cast{};
		}
		return *(HspObjectPath::StaticVar const*)this;
	}
};

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
