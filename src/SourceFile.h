#pragma once

#include <vector>
#include "encoding.h"

// HSP のスクリプトファイルを表す。
class SourceFile {
	OsString full_path_;
	OsString content_;
	std::vector<OsString> lines_;

public:
	SourceFile(OsString&& full_path, OsString&& content);

	auto full_path() const -> OsStringView {
		return full_path_.as_ref();
	}

	auto content() const->OsStringView {
		return content_.as_ref();
	}

	// 指定した行の文字列を取得する。
	// 編集中で保存されていないスクリプトを実行しているときは、行番号が行数を超える可能性があるので注意。
	auto line_at(std::size_t line_index) const->OsStringView {
		if (line_index >= lines_.size()) {
			return OsStringView{ TEXT("") };
		}

		return lines_[line_index].as_ref();
	}
};
