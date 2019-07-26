#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "encoding.h"

class SourceFile;
class SourceFileId;
class SourceFileRepository;
class SourceFileResolver;

// ファイル参照名を絶対パスに対応付ける処理を担当する。
class SourceFileResolver {
	// ファイルを探す基準となるディレクトリの集合。
	std::unordered_set<OsString> dirs_;

	// 解決すべきファイル参照名の集合。
	std::unordered_set<std::string> file_ref_names_;

public:
	void add_known_dir(OsString&& dir);

	void add_file_ref_name(std::string&& file_ref_name);

	auto resolve()->SourceFileRepository;
};

// ファイルID、ファイル参照名、絶対パス、ソースファイルを持つ。
class SourceFileRepository {
	std::vector<SourceFile> source_files_;

	// ファイル参照名 → ファイルID
	std::unordered_map<std::string, SourceFileId> file_map_;

public:
	SourceFileRepository(std::vector<SourceFile>&& source_files, std::unordered_map<std::string, SourceFileId>&& file_map)
		: source_files_(std::move(source_files))
		, file_map_(std::move(file_map))
	{
	}

	auto file_count() const -> std::size_t {
		return source_files_.size();
	}

	auto file_ref_name_to_file_id(char const* file_ref_name) const->std::optional<SourceFileId>;

	auto file_ref_name_to_full_path(char const* file_ref_name) const->std::optional<OsStringView>;

	auto file_ref_name_to_content(char const* file_ref_name)->std::optional<Utf8StringView>;

	auto file_ref_name_to_line_at(char const* file_ref_name, std::size_t line_index)->std::optional<Utf8StringView>;

	auto file_to_full_path(SourceFileId const& file_id) const->std::optional<OsStringView>;

	auto file_to_full_path_as_utf8(SourceFileId const& file_id) const->std::optional<Utf8StringView>;

	auto file_to_content(SourceFileId const& file_id)->std::optional<Utf8StringView>;

	auto file_to_line_at(SourceFileId const& file_id, std::size_t line_index)->std::optional<Utf8StringView>;
};

// ソースファイルの管理番号
class SourceFileId {
	std::size_t id_;

public:
	explicit SourceFileId(std::size_t id)
		: id_(id)
	{
	}

	auto id() const -> std::size_t {
		return id_;
	}

	auto full_path(SourceFileRepository const& files) const -> std::optional<OsStringView> {
		return files.file_to_full_path(*this);
	}

	auto content(SourceFileRepository& files) const -> std::optional<Utf8StringView> {
		return files.file_to_content(*this);
	}

	auto line_at(std::size_t line_index, SourceFileRepository& files) const -> std::optional<Utf8StringView> {
		return files.file_to_line_at(*this, line_index);
	}
};

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
