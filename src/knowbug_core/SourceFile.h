#pragma once

#include <vector>
#include "encoding.h"

// HSP のスクリプトファイルを表す。
class SourceFile {
	OsString full_path_;
	Utf8String content_;
	std::vector<Utf8StringView> lines_;

public:
	SourceFile(OsString&& full_path, Utf8String&& content);

	auto full_path() const -> OsStringView {
		return as_view(full_path_);
	}

	auto content() const->Utf8StringView {
		return as_view(content_);
	}

	// 指定した行の文字列を取得する。
	// 編集中で保存されていないスクリプトを実行しているときは、行番号が行数を超える可能性があるので注意。
	auto line_at(std::size_t line_index) const->Utf8StringView;
};
