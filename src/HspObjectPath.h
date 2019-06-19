#pragma once

#include <string>

class HspObjects;

// HSP のオブジェクトを指し示すルートからの経路
// 実際には抽象的なオブジェクトだが、まだ静的変数しか考えてない。
class HspObjectPath {
	std::size_t static_var_id_;

public:
	HspObjectPath(std::size_t static_var_id)
		: static_var_id_(static_var_id)
	{
	}

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto name(HspObjects& objects) const -> std::string;

	bool is_array(HspObjects& objects) const;
};
