#include "pch.h"
#include <array>
#include "string_split.h"
#include "SourceFile.h"

// NOTE: ifstream が string_view を受け付けないので OsString への参照を取っている。
static auto load_text_file(OsString const& file_path) -> Utf8String {
	auto ifs = std::ifstream{ file_path };
	if (!ifs.is_open()) {
		return to_owned(as_utf8(u8""));
	}

	// NOTE: ソースコードの文字コードが HSP ランタイムの文字コードと同じとは限らない。
	auto content = as_hsp(std::string{ std::istreambuf_iterator<char>{ ifs }, {} });
	return to_utf8(content);
}

// 行ごとに分割する。ただし各行の字下げは無視する。
static auto split_by_lines(Utf8StringView const& str) -> std::vector<Utf8StringView> {
	auto lines = std::vector<Utf8StringView>{};

	for (auto&& line : StringLines{ str }) {
		auto i = std::size_t{};
		while (i < line.size() && (line[i] == (Utf8Char)' ' || line[i] == (Utf8Char)'\t')) {
			i++;
		}

		lines.emplace_back(line.substr(i));
	}

	return lines;
}

SourceFile::SourceFile(OsString&& full_path)
	: full_path_(std::move(full_path))
	, loaded_(false)
	, content_()
	, lines_()
{
}

auto SourceFile::content() -> Utf8StringView {
	load();

	return content_;
}

auto SourceFile::line_at(std::size_t line_index) -> Utf8StringView {
	load();

	if (line_index >= lines_.size()) {
		return as_utf8(u8"");
	}

	return lines_[line_index];
}

void SourceFile::load() {
	if (loaded_) {
		return;
	}

	content_ = load_text_file(full_path_);
	lines_ = split_by_lines(as_view(content_));
	loaded_ = true;
}
