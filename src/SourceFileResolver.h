#pragma once

#include <map>
#include <optional>
#include <set>
#include "encoding.h"
#include "SourceFile.h"

namespace hpiutil {
	class DInfo;
}

class SourceFileResolver;

// ファイル参照名と絶対パスの対応、および各ソースコードのキャッシュを持つ。
class SourceFileResolver {
	bool resolution_done_;

	// ファイルを探す基準となるディレクトリの集合
	std::set<OsString> dirs_;

	// ファイル参照名から絶対パスのマップ
	std::map<OsString, OsString> full_paths_;

	// 絶対パスからソースファイルへのマップ
	std::map<OsString, std::shared_ptr<SourceFile>> source_files_;

	hpiutil::DInfo const& debug_segment_;

public:
	SourceFileResolver(OsString&& common_path, hpiutil::DInfo const& debug_segment);

	// ファイル参照名の絶対パスを検索する。
	auto find_full_path(OsStringView const& file_ref_name, OsStringView& out_full_path)->bool;

	auto find_script_content(OsStringView const& file_ref_name, OsStringView& out_content)->bool;

	auto find_script_line(OsStringView const& file_ref_name, std::size_t line_index)->std::optional<OsStringView>;

private:
	auto resolve_file_ref_names()->void;

	auto find_full_path_core(OsStringView const& file_ref_name)->std::optional<OsStringView>;

	auto find_source_file(OsStringView const& file_ref_name) -> std::optional<std::shared_ptr<SourceFile>>;
};
