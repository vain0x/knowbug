#include "pch.h"
#include <array>
#include "string_split.h"
#include "SourceFile.h"

// 行ごとに分割する。ただし各行の字下げは無視する。
static auto split_by_lines(OsStringView const& str) -> std::vector<OsString> {
	auto lines = std::vector<OsString>{};

	for (auto&& line : StringLines{ str }) {
		auto i = std::size_t{};
		while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
			i++;
		}

		lines.emplace_back(to_owned(line.substr(i)));
	}

	return lines;
}

SourceFile::SourceFile(OsString&& full_path, OsString&& content)
	: full_path_(std::move(full_path))
	, content_(std::move(content))
{
	lines_ = split_by_lines(as_view(content_));
}
