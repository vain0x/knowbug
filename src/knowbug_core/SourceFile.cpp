#include <array>
#include "SourceFile.h"

// 行ごとに分割する。ただし各行の字下げは無視する。
static auto split_by_lines(OsStringView const& str) -> std::vector<OsString> {
	auto lines = std::vector<OsString>{};

	// いま見ている行の行頭の位置
	auto head = std::size_t{};

	// いま見ている位置
	auto i = std::size_t{};

	while (i <= str.size()) {
		// EOF または行末
		if (i == str.size() || str[i] == '\n') {
			// 行末の位置
			auto r = i;
			if (r >= 1 && str[r - 1] == '\r') {
				r--;
			}

			// 行頭の位置 (字下げを除く)
			auto l = head;
			while (l < r && (str[l] == ' ' || str[l] == '\t')) {
				l++;
			}

			// l..r が1行の範囲
			lines.emplace_back(OsString{ str.data() + l, str.data() + r });

			// LF を飛ばす。
			i++;
			head = i;
			continue;
		}

		i++;
	}

	return lines;
}

SourceFile::SourceFile(OsString&& full_path, OsString&& content)
	: full_path_(std::move(full_path))
	, content_(std::move(content))
{
	lines_ = split_by_lines(as_view(content_));
}
