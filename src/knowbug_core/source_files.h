#pragma once

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "encoding.h"
#include "test_suite.h"

class SourceFile;
class SourceFileId;
class SourceFileRepository;
class SourceFileResolver;

extern void source_files_tests(Tests& tests);

// -----------------------------------------------
// ファイルシステム
// -----------------------------------------------

// ファイルシステムへの操作を表す。
// (ファイルシステムにアクセスすることなくファイル操作を行うコードをテストするための抽象化層。)
class FileSystemApi {
public:
	class SearchFileResult {
	public:
		// ファイルが含まれるディレクトリへの絶対パス
		OsString dir_path_;

		// 見つかったファイルへの絶対パス
		OsString full_path_;
	};

	virtual auto read_all_text(OsString const& file_path)->std::optional<std::string> = 0;

	virtual auto search_file_from_dir(OsStringView file_name, OsStringView base_dir)->std::optional<SearchFileResult> = 0;

	virtual auto search_file_from_current_dir(OsStringView file_name)->std::optional<SearchFileResult> = 0;
};

// FileSysetmApi を Windows のファイル操作 API を使って実装したもの。
class WindowsFileSystemApi
	: public FileSystemApi
{
public:
	auto read_all_text(OsString const& file_path)->std::optional<std::string> override;

	auto search_file_from_dir(OsStringView file_name, OsStringView base_dir)->std::optional<SearchFileResult> override;

	auto search_file_from_current_dir(OsStringView file_name)->std::optional<SearchFileResult> override;
};

// ファイル参照名を絶対パスに対応付ける処理を担当する。
class SourceFileResolver {
	// ファイルを探す基準となるディレクトリの集合。
	std::unordered_set<OsString> dirs_;

	// 解決すべきファイル参照名の集まり。
	std::vector<std::string> file_ref_names_;

	FileSystemApi& fs_;

public:
	explicit SourceFileResolver(FileSystemApi& fs)
		: dirs_()
		, file_ref_names_()
		, fs_(fs)
	{
	}

	// ファイルを探す基準となるディレクトリを登録する。
	void add_known_dir(OsString&& dir);

	// 解決すべきファイル参照名を登録する。(重複登録は無視される。)
	void add_file_ref_name(std::string&& file_ref_name);

	// 重複して登録されたファイル参照名を削除する。
	void dedup();

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

	auto file_ref_name_to_content(char const* file_ref_name)->std::optional<std::u8string_view>;

	auto file_ref_name_to_line_at(char const* file_ref_name, std::size_t line_index)->std::optional<std::u8string_view>;

	auto file_to_full_path(SourceFileId const& file_id) const->std::optional<OsStringView>;

	auto file_to_full_path_as_utf8(SourceFileId const& file_id) const->std::optional<std::u8string_view>;

	auto file_to_content(SourceFileId const& file_id)->std::optional<std::u8string_view>;

	auto file_to_line_at(SourceFileId const& file_id, std::size_t line_index)->std::optional<std::u8string_view>;
};

// ソースファイルの管理番号
class SourceFileId {
	std::size_t id_;

public:
	explicit SourceFileId(std::size_t id)
		: id_(id)
	{
	}

	auto operator ==(SourceFileId other) const -> bool {
		return id_ == other.id_;
	}

	auto id() const -> std::size_t {
		return id_;
	}

	auto full_path(SourceFileRepository const& files) const -> std::optional<OsStringView> {
		return files.file_to_full_path(*this);
	}

	auto content(SourceFileRepository& files) const -> std::optional<std::u8string_view> {
		return files.file_to_content(*this);
	}

	auto line_at(std::size_t line_index, SourceFileRepository& files) const -> std::optional<std::u8string_view> {
		return files.file_to_line_at(*this, line_index);
	}
};

// HSP のソースファイル (スクリプトファイル) を表す。
class SourceFile {
	OsString full_path_;

	std::u8string full_path_as_utf8_;

	// ソースファイルの内容を読むべきファイルの絶対パス。
	std::optional<OsString> content_file_path_;

	// ソースファイルの中身がロード済みなら true
	bool loaded_;

	// ソースファイルの中身。loaded_=true のときだけ有効。
	// 最初に参照される際にロードする。
	std::u8string content_;

	// ソースファイルの中身を行ごとに分割し、字下げを取り除いたもの。
	std::vector<std::u8string_view> lines_;

	FileSystemApi& fs_;

public:
	SourceFile(OsString&& full_path, FileSystemApi& fs);

	// 絶対パス
	auto full_path() const -> OsStringView {
		return full_path_;
	}

	// 絶対パス (UTF-8 エンコーディング)
	auto full_path_as_utf8() const -> std::u8string_view {
		return full_path_as_utf8_;
	}

	// ソースファイルの中身を取得する。
	auto content()->std::u8string_view;

	// ソースファイルの指定した行の文字列 (字下げを除く) を取得する。
	auto line_at(std::size_t line_index)->std::optional<std::u8string_view>;

	auto set_content_file_path(OsString&& content_file_path) {
		content_file_path_ = std::move(content_file_path);
	}

private:
	void load();
};
