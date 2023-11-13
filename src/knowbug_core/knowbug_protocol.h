// knowbug メッセージ転送プロトコル

#pragma once

#include <optional>
#include <unordered_map>
#include "encoding.h"

class Tests;

class KnowbugMessage {
	using Pair = std::pair<std::u8string, std::u8string>;
	using Assoc = std::vector<Pair>;

	Assoc assoc_;

public:
	static auto new_with_method(std::u8string method) -> KnowbugMessage {
		auto it = KnowbugMessage{};
		it.insert(std::u8string{ u8"method" }, std::move(method));
		return it;
	}

	auto size() const->std::size_t {
		return assoc_.size();
	}

	auto method() const->std::u8string_view {
		if (assoc_.empty()) {
			assert(false && u8"missing method");
			return u8"NO_METHOD";
		}

		return assoc_[0].second;
	}

	auto get(std::u8string_view key) const->std::optional<std::u8string_view> {
		for (auto&& pair : assoc_) {
			if (pair.first == key) {
				return pair.second;
			}
		}

		return std::nullopt;
	}

	auto get_int(std::u8string_view key) const->std::optional<int> {
		auto value_opt = get(key);
		if (!value_opt) {
			return std::nullopt;
		}

		return std::atol(as_native(*value_opt).data());
	}

	auto get_bool(std::u8string_view key) const->std::optional<bool> {
		auto value_opt = get(key);
		if (!value_opt) {
			return std::nullopt;
		}

		return *value_opt == u8"true"
			|| *value_opt == u8"yes"
			|| *value_opt == u8"0";
	}

	void insert(std::u8string key, std::u8string value) {
		assoc_.push_back(Pair{ std::move(key),std::move(value) });
	}

	void insert_int(std::u8string key, int value) {
		insert(std::move(key), as_utf8(std::to_string(value)));
	}

	void insert_bool(std::u8string key, bool value) {
		insert(std::move(key), std::u8string{ value ? u8"true" : u8"false" });
	}

	auto begin() const -> Assoc::const_iterator {
		return assoc_.begin();
	}

	auto end() const -> Assoc::const_iterator {
		return assoc_.end();
	}
};

// メッセージを解析する。
extern auto knowbug_protocol_parse(std::u8string_view body) -> std::optional<KnowbugMessage>;

// メッセージを構築する。
extern auto knowbug_protocol_serialize(KnowbugMessage const& message)->std::u8string;

extern void knowbug_protocol_tests(Tests& tests);
