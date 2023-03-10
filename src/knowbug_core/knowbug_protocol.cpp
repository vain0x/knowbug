#include "pch.h"
#include "encoding.h"
#include "knowbug_protocol.h"
#include "string_format.h"
#include "string_split.h"
#include "test_suite.h"
#include "transfer_protocol.h"

static auto char_is_space(Utf8Char c) -> bool {
	return c == Utf8Char{ u8' ' } || c == Utf8Char{ u8'\t' };
}

static auto string_trim_start(Utf8StringView str) -> Utf8StringView {
	auto i = std::size_t{};
	while (i < str.size() && char_is_space(str[i])) {
		i++;
	}
	return str.substr(i);
}

static auto string_trim_end(Utf8StringView str) -> Utf8StringView {
	auto len = str.size();
	while (len >= 1 && char_is_space(str[len - 1])) {
		len--;
	}
	return str.substr(0, len);
}

static auto string_trim(Utf8StringView str) -> Utf8StringView {
	return string_trim_start(string_trim_end(str));
}

static auto parse_hex_digit(Utf8Char c) -> std::size_t {
	if (Utf8Char{ u8'0' } <= c && c <= Utf8Char{ u8'9' }) {
		return (std::size_t)((char)c - u8'0');
	}

	if (Utf8Char{ u8'A' } <= c && c <= Utf8Char{ u8'F' }) {
		return (std::size_t)((char)c - u8'A' + 10);
	}

	if (Utf8Char{ u8'a' } <= c && c <= Utf8Char{ u8'f' }) {
		return (std::size_t)((char)c - u8'a' + 10);
	}

	assert(false && u8"not hex digit");
	return std::size_t{};
}

static auto escape(Utf8StringView str) -> Utf8String {
	auto output = Utf8String{};

	auto i = std::size_t{};
	auto last = i;

	while (i < str.size()) {
		auto c = (char)str[i];
		if (c == u8'\0' || c == u8'\t' || c == u8'\r' || c == u8'\n' || c == u8' '
			|| c == u8'=' || c == u8'"' || c == u8'\\') {
			output += str.substr(last, i - last);
			output += as_utf8(strf("\\x%02x", c));
			i++;
			last = i;
			continue;
		}

		i++;
	}
	output += str.substr(last, i - last);
	return output;
}

static auto unescape(Utf8StringView str) -> Utf8String {
	auto output = Utf8String{};

	auto i = std::size_t{};
	auto last = i;

	while (i < str.size() && str[i] != Utf8Char{ u8'\0' }) {
		if (str[i] == Utf8Char{ u8'\\' } && i + 4 <= str.size()) {
			output += str.substr(last, i - last);

			assert(str[i + 1] == Utf8Char{ u8'x' });

			auto x1 = parse_hex_digit(str[i + 2]);
			auto x2 = parse_hex_digit(str[i + 3]);

			auto c = (Utf8Char)((x1 * 16) | x2);
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

auto knowbug_protocol_parse(Utf8String& buffer)->std::optional<KnowbugMessage> {
	auto body = Utf8String{};

	if (!transfer_protocol_parse(body, buffer)) {
		return std::nullopt;
	}

	auto message = KnowbugMessage{};

	for (auto&& line : StringLines<Utf8Char>{ body }) {
		auto trimmed_line = string_trim(line);
		if (trimmed_line.empty()) {
			continue;
		}

		auto colon = trimmed_line.find(Utf8Char{ u8'=' });
		if (colon == Utf8String::npos) {
			assert(false && u8"missing = in line");
			continue;
		}

		auto key = Utf8String{ string_trim(trimmed_line.substr(0, colon)) };
		auto value = Utf8String{ string_trim(trimmed_line.substr(colon + 1)) };

		message.insert(unescape(key), unescape(value));
	}

	if (message.size() == 0 || !message.get(as_utf8(u8"method"))) {
		assert(false && u8"missing method key");
		return std::nullopt;
	}

	return message;
}

auto knowbug_protocol_serialize(KnowbugMessage const& message) -> Utf8String {
	auto body = Utf8String{};
	for (auto&& pair : message) {
		body += escape(pair.first);
		body += as_utf8(u8" = ");
		body += escape(pair.second);
		body += as_utf8(u8"\r\n");
	}

	auto output = Utf8String{};
	output += as_utf8(u8"Content-Length: ");
	output += as_utf8(std::to_string(body.size()));
	output += as_utf8(u8"\r\n\r\n");
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
				Utf8String{ as_utf8(u8"method") },
				Utf8String{ as_utf8(u8"foo_event") }
			);

			message.insert(
				Utf8String{ as_utf8(u8"assignment") },
				Utf8String{ as_utf8(u8"yen = \"\\\"") }
			);

			message.insert(
				Utf8String{ as_utf8(u8"crlf") },
				Utf8String{ as_utf8(u8"\r\n") }
			);

			auto output = knowbug_protocol_serialize(message);
			auto output_native = as_native(output);

			return t.eq(
				output,
				as_utf8(u8"Content-Length: 79\r\n\r\nmethod = foo_event\r\nassignment = yen\\x20\\x3d\\x20\\x22\\x5c\\x22\r\ncrlf = \\x0d\\x0a\r\n")
			);
		});

	suite.test(
		u8"basic",
		[](TestCaseContext& t) {
			auto buffer = Utf8String{ as_utf8(u8"Content-Length: 79\r\n\r\nmethod = foo_event\r\nassignment = yen\\x20\\x3d\\x20\\x22\\x5c\\x22\r\ncrlf = \\x0d\\x0a\r\n") };
			auto message_opt = knowbug_protocol_parse(buffer);

			auto method = as_native(message_opt->method());
			auto assignment = as_native(*message_opt->get(as_utf8(u8"assignment")));

			return t.eq(message_opt.has_value(), true)
				&& t.eq(message_opt->size(), 3)
				&& t.eq(message_opt->method(), as_utf8(u8"foo_event"))
				&& t.eq(*message_opt->get(as_utf8(u8"assignment")), as_utf8(u8"yen = \"\\\""))
				&& t.eq(*message_opt->get(as_utf8(u8"crlf")), as_utf8(u8"\r\n"));
		});
}
