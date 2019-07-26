#pragma once

#include <vector>
#include "encoding.h"

// HSP のソースファイル (スクリプトファイル) を表す。
class SourceFile {
	OsString full_path_;

	Utf8String full_path_as_utf8_;

	// ソースファイルの中身がロード済みなら true
	bool loaded_;

	// ソースファイルの中身。loaded_=true のときだけ有効。
	// 最初に参照される際にロードする。
	Utf8String content_;

	// ソースファイルの中身を行ごとに分割し、字下げを取り除いたもの。
	std::vector<Utf8StringView> lines_;

public:
	explicit SourceFile(OsString&& full_path);

	// 絶対パス
	auto full_path() const -> OsStringView {
		return as_view(full_path_);
	}

	// 絶対パス (UTF-8 エンコーディング)
	auto full_path_as_utf8() const -> Utf8StringView {
		return as_view(full_path_as_utf8_);
	}

	// ソースファイルの中身を取得する。
	auto content()->Utf8StringView;

	// ソースファイルの指定した行の文字列 (字下げを除く) を取得する。
	auto line_at(std::size_t line_index)->std::optional<Utf8StringView>;

private:
	void load();
};
