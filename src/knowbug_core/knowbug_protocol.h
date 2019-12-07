// knowbug メッセージ転送プロトコル

#pragma once

#include <optional>
#include <unordered_map>
#include "encoding.h"

class Tests;

class KnowbugMessage {
	using Pair = std::pair<Utf8String, Utf8String>;
	using Assoc = std::vector<Pair>;

	Assoc assoc_;

public:
	static auto new_with_method(Utf8String method) -> KnowbugMessage {
		auto it = KnowbugMessage{};
		it.insert(Utf8String{ as_utf8(u8"method") }, std::move(method));
		return it;
	}

	auto size() const->std::size_t {
		return assoc_.size();
	}

	auto method() const->Utf8StringView {
		if (assoc_.empty()) {
			assert(false && u8"missing method");
			return as_utf8(u8"NO_METHOD");
		}

		return assoc_[0].second;
	}

	auto get(Utf8StringView key) const->std::optional<Utf8StringView> {
		for (auto&& pair : assoc_) {
			if (pair.first == key) {
				return pair.second;
			}
		}

		return std::nullopt;
	}

	auto get_int(Utf8StringView key) const->std::optional<int> {
		auto value_opt = get(key);
		if (!value_opt) {
			return std::nullopt;
		}

		return std::atol(as_native(*value_opt).data());
	}

	auto get_bool(Utf8StringView key) const->std::optional<bool> {
		auto value_opt = get(key);
		if (!value_opt) {
			return std::nullopt;
		}

		return *value_opt == as_utf8(u8"true")
			|| *value_opt == as_utf8(u8"yes")
			|| *value_opt == as_utf8(u8"0");
	}

	void insert(Utf8String key, Utf8String value) {
		assoc_.push_back(Pair{ std::move(key),std::move(value) });
	}

	void insert_int(Utf8String key, int value) {
		insert(std::move(key), as_utf8(std::to_string(value)));
	}

	void insert_bool(Utf8String key, bool value) {
		insert(std::move(key), Utf8String{ as_utf8(value ? u8"true" : u8"false") });
	}

	auto begin() const -> Assoc::const_iterator {
		return assoc_.begin();
	}

	auto end() const -> Assoc::const_iterator {
		return assoc_.end();
	}
};

// バッファーからメッセージを取り出す。
// 取り出したら true を返し、バッファーからメッセージを取り除き、ボディー部分を解析したデータを body にコピーする。
extern auto knowbug_protocol_parse(Utf8String& buffer) -> std::optional<KnowbugMessage>;

// メッセージを構築する。
extern auto knowbug_protocol_serialize(KnowbugMessage const& message)->Utf8String;

extern void knowbug_protocol_tests(Tests& tests);
