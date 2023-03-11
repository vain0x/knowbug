#include "pch.h"
#include "encoding.h"
#include "knowbug_protocol.h"
#include "test_suite.h"

static auto char_is_space(Utf8Char c) -> bool {
	return c == u8' ' || c == u8'\t';
}

static auto skip_spaces(Utf8StringView str, std::size_t start_index) -> std::size_t {
	auto i = start_index;
	while (i < str.size() && char_is_space(str[i])) {
		i++;
	}
	return i;
}

static auto skip_others(Utf8StringView str, std::size_t start_index, Utf8Char c) -> std::size_t {
	auto i = start_index;
	while (i < str.size() && str[i] != c && str[i] != u8'\r') {
		i++;
	}
	return i;
}

auto transfer_protocol_parse(Utf8String& body, Utf8String& buffer) -> bool {
	auto index = std::size_t{};
	auto content_length_opt = std::optional<std::size_t>{};
	auto has_body = false;

	while (index < buffer.size()) {
		auto key_start = skip_spaces(buffer.data(), index);
		auto key_end = skip_others(buffer.data(), key_start, u8':');

		assert(index <= key_start);
		assert(key_start <= key_end);
		assert(key_end <= buffer.size());

		// ボディー直前の \r\n のとき
		if (key_start == key_end) {
			has_body = true;
			index = std::min(key_end + 2, buffer.size());
			break;
		}

		// ':' の前までしかデータが来ていないとき
		if (key_end == buffer.size() || buffer[key_end] == u8'\r') {
			break;
		}

		auto value_start = skip_spaces(buffer.data(), key_end + 1);
		auto value_end = skip_others(buffer.data(), value_start, u8'\r');

		assert(key_end < value_start);
		assert(value_start <= value_end);
		assert(value_end <= buffer.size());

		// ボディーの前までしかデータが来ていないとき
		if (value_end + 2 > buffer.size()) {
			break;
		}

		index = value_end + 2;

		// ヘッダーを解釈する。
		auto header_key = Utf8StringView{ buffer.data() + key_start, key_end - key_start };
		auto header_value = Utf8StringView{ buffer.data() + value_start, value_end - value_start };

		if (header_key == u8"Content-Length") {
			content_length_opt = (std::size_t)std::atoll(as_native(header_value).data());
			continue;
		}

		// 不明なヘッダーを無視する。
	}

	if (!has_body) {
		// ヘッダーの途中までしかデータが来ていないとき
		return false;
	}

	if (!content_length_opt) {
		// Content-Length がないとき
		assert(false);
		return false;
	}

	if (buffer.size() < index + *content_length_opt) {
		// ボディーの途中までしかデータが来ていないとき
		return false;
	}

	body = Utf8StringView{ buffer.data() + index, *content_length_opt };

	// バッファーを詰める。
	auto rest_size = buffer.size() - (index + body.size());
	if (buffer.size() > 0) {
		std::memmove(buffer.data(), buffer.data() + index + body.size(), rest_size);
	}
	buffer.resize(rest_size);

	return true;
}

void transfer_protocol_tests(Tests& tests) {
	auto& suite = tests.suite(u8"transfer_protocol");

	suite.test(
		u8"中途半端なケースとぴったりのケース",
		[](TestCaseContext& t) {
			auto buffer = Utf8String{};
			auto body = Utf8String{};

			auto success = transfer_protocol_parse(body, buffer);
			if (!t.eq(success, false)) {
				return false;
			}

			buffer += u8"Content-Length";
			success = transfer_protocol_parse(body, buffer);
			if (!t.eq(success, false)) {
				return false;
			}

			buffer += u8": 15\r\n";
			success = transfer_protocol_parse(body, buffer);
			if (!t.eq(success, false)) {
				return false;
			}

			buffer += u8"\r\nHello, ";
			success = transfer_protocol_parse(body, buffer);
			if (!t.eq(success, false)) {
				return false;
			}

			buffer += u8"world!\r\n";
			success = transfer_protocol_parse(body, buffer);
			return t.eq(success, true)
				&& t.eq(body, as_utf8(u8"Hello, world!\r\n"))
				&& t.eq(buffer.empty(), true);
		});

	suite.test(
		u8"2つのメッセージが連結されたバッファーのパース",
		[](TestCaseContext& t) {
			auto buffer = Utf8String{ u8"Content-Length: 15\r\n\r\nHello, world!\r\nContent-Length: 11\r\n\r\nGood bye!\r\n" };
			auto body = Utf8String{};

			auto success = transfer_protocol_parse(body, buffer);
			if (!t.eq(success, true)
				|| !t.eq(body, as_utf8(u8"Hello, world!\r\n"))
				|| !t.eq(buffer.empty(), false)) {
				return false;
			}

			success = transfer_protocol_parse(body, buffer);
			return t.eq(success, true)
				&& t.eq(body, as_utf8(u8"Good bye!\r\n"))
				&& t.eq(buffer.empty(), true);
		});
}
