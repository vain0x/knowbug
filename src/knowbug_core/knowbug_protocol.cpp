#include "pch.h"
#include "encoding.h"
#include "knowbug_protocol.h"
#include "string_format.h"
#include "string_split.h"
#include "test_suite.h"
#include "transfer_protocol.h"

static auto char_is_space(char8_t c) -> bool {
	return c == u8' ' || c == u8'\t';
}

static auto string_trim_start(std::u8string_view str) -> std::u8string_view {
	auto i = std::size_t{};
	while (i < str.size() && char_is_space(str[i])) {
		i++;
	}
	return str.substr(i);
}

static auto string_trim_end(std::u8string_view str) -> std::u8string_view {
	auto len = str.size();
	while (len >= 1 && char_is_space(str[len - 1])) {
		len--;
	}
	return str.substr(0, len);
}

static auto string_trim(std::u8string_view str) -> std::u8string_view {
	return string_trim_start(string_trim_end(str));
}

static auto parse_hex_digit(char8_t c) -> std::size_t {
	if (u8'0' <= c && c <= u8'9') {
		return (std::size_t)(c - u8'0');
	}

	if (u8'A' <= c && c <= u8'F') {
		return (std::size_t)(c - u8'A' + 10);
	}

	if (u8'a' <= c && c <= u8'f') {
		return (std::size_t)(c - u8'a' + 10);
	}

	assert(false && u8"not hex digit");
	return std::size_t{};
}

static auto escape(std::u8string_view str) -> std::u8string {
	auto output = std::u8string{};
	char temp[8] = "";

	auto i = std::size_t{};
	auto last = i;

	while (i < str.size()) {
		auto c = str[i];
		if (c == u8'\0' || c == u8'\t' || c == u8'\r' || c == u8'\n' || c == u8' '
			|| c == u8'=' || c == u8'"' || c == u8'\\') {
			output += str.substr(last, i - last);
			sprintf_s(temp, "\\x%02x", (std::uint8_t)c);
			output += ascii_as_utf8(temp);
			i++;
			last = i;
			continue;
		}

		i++;
	}
	output += str.substr(last, i - last);
	return output;
}

static auto unescape(std::u8string_view str) -> std::u8string {
	auto output = std::u8string{};

	auto i = std::size_t{};
	auto last = i;

	while (i < str.size() && str[i] != u8'\0') {
		if (str[i] == u8'\\' && i + 4 <= str.size()) {
			output += str.substr(last, i - last);

			assert(str[i + 1] == u8'x');

			auto x1 = parse_hex_digit(str[i + 2]);
			auto x2 = parse_hex_digit(str[i + 3]);

			auto c = (char8_t)((x1 * 16) | x2);
			output.push_back(c);

			i += 4;
			last = i;
			continue;
		}

		i++;
	}

	output += str.substr(last, i - last);
	return output;
}

auto knowbug_protocol_parse(std::u8string& buffer)->std::optional<KnowbugMessage> {
	auto body = std::u8string{};

	if (!transfer_protocol_parse(body, buffer)) {
		return std::nullopt;
	}

	auto message = KnowbugMessage{};

	for (auto&& line : StringLines<char8_t>{ body }) {
		auto trimmed_line = string_trim(line);
		if (trimmed_line.empty()) {
			continue;
		}

		auto colon = trimmed_line.find(u8'=');
		if (colon == std::u8string::npos) {
			assert(false && u8"missing = in line");
			continue;
		}

		auto key = std::u8string{ string_trim(trimmed_line.substr(0, colon)) };
		auto value = std::u8string{ string_trim(trimmed_line.substr(colon + 1)) };

		message.insert(unescape(key), unescape(value));
	}

	if (message.size() == 0 || !message.get(u8"method")) {
		assert(false && u8"missing method key");
		return std::nullopt;
	}

	return message;
}

auto knowbug_protocol_serialize(KnowbugMessage const& message) -> std::u8string {
	auto body = std::u8string{};
	for (auto&& pair : message) {
		body += escape(pair.first);
		body += u8" = ";
		body += escape(pair.second);
		body += u8"\r\n";
	}

	auto output = std::u8string{};
	output += u8"Content-Length: ";
	output += as_utf8(std::to_string(body.size()));
	output += u8"\r\n\r\n";
	output += body;
	return output;
}

void knowbug_protocol_tests(Tests& tests) {
	auto& suite = tests.suite(u8"knowbug_protocol");

	suite.test(
		u8"basic",
		[](TestCaseContext& t) {
			auto message = KnowbugMessage{};
			message.insert(
				std::u8string{ u8"method" },
				std::u8string{ u8"foo_event" }
			);

			message.insert(
				std::u8string{ u8"assignment" },
				std::u8string{ u8"yen = \"\\\"" }
			);

			message.insert(
				std::u8string{ u8"crlf" },
				std::u8string{ u8"\r\n" }
			);

			auto output = knowbug_protocol_serialize(message);
			auto output_native = as_native(output);

			return t.eq(
				output,
				u8"Content-Length: 79\r\n\r\nmethod = foo_event\r\nassignment = yen\\x20\\x3d\\x20\\x22\\x5c\\x22\r\ncrlf = \\x0d\\x0a\r\n"
			);
		});

	suite.test(
		u8"basic",
		[](TestCaseContext& t) {
			auto buffer = std::u8string{ u8"Content-Length: 79\r\n\r\nmethod = foo_event\r\nassignment = yen\\x20\\x3d\\x20\\x22\\x5c\\x22\r\ncrlf = \\x0d\\x0a\r\n" };
			auto message_opt = knowbug_protocol_parse(buffer);

			auto method = as_native(message_opt->method());
			auto assignment = as_native(*message_opt->get(u8"assignment"));

			return t.eq(message_opt.has_value(), true)
				&& t.eq(message_opt->size(), 3)
				&& t.eq(message_opt->method(), u8"foo_event")
				&& t.eq(*message_opt->get(u8"assignment"), u8"yen = \"\\\"")
				&& t.eq(*message_opt->get(u8"crlf"), u8"\r\n");
		});
}
