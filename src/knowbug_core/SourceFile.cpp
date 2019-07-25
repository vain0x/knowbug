#include "pch.h"
#include <array>
#include "string_split.h"
#include "SourceFile.h"

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

SourceFile::SourceFile(OsString&& full_path, Utf8String&& content)
	: full_path_(std::move(full_path))
	, content_(std::move(content))
{
	lines_ = split_by_lines(as_view(content_));
}

auto SourceFile::line_at(std::size_t line_index) const->Utf8StringView {
	if (line_index >= lines_.size()) {
		return as_utf8(u8"");
	}

	return lines_[line_index];
}
