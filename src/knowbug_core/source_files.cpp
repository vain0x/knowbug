#include "pch.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include "source_files.h"
#include "string_split.h"

// 改行コードを CRLF に固定する。
static auto normalize_lines(std::u8string_view text) -> std::u8string {
	auto output = std::u8string{};
	auto first = true;

	for (auto&& line : StringLines{ text }) {
		if (!std::exchange(first, false)) {
			output += u8"\r\n";
		}

		output += line;
	}

	return output;
}

// -----------------------------------------------
// ファイルシステム
// -----------------------------------------------

static auto read_all_text(OsString const& file_path)->std::optional<std::string> {
	auto ifs = std::ifstream{ file_path }; // NOTE: OsStringView に対応していない。
	if (!ifs.is_open()) {
		return std::nullopt;
	}
	return std::string{ std::istreambuf_iterator<char>{ ifs }, {} };
}

// 指定したディレクトリを基準として、指定した名前または相対パスのファイルを検索する。
// 結果として、フルパスと、パス中のディレクトリの部分を返す。
// base_dir_opt=nullopt のときは、カレントディレクトリで検索する。
static auto search_file_from_dir(
	OsStringView const& file_ref,
	std::optional<OsStringView> base_dir_opt
) -> std::optional<FileSystemApi::SearchFileResult> {
	static auto const EXTENSION_PTR = LPCTSTR{};
	static auto const CURRENT_DIR = LPCTSTR{};

	auto file_name_ptr = LPTSTR{};
	auto full_path_buf = std::array<TCHAR, MAX_PATH>{};

	auto base_dir_ptr = base_dir_opt ? base_dir_opt->data() : CURRENT_DIR;
	auto succeeded =
		SearchPath(
			base_dir_ptr, file_ref.data(), EXTENSION_PTR,
			(DWORD)full_path_buf.size(), full_path_buf.data(), &file_name_ptr
		) != 0;
	if (!succeeded) {
		return std::nullopt;
	}

	assert(full_path_buf.data() <= file_name_ptr && file_name_ptr <= full_path_buf.data() + full_path_buf.size());
	auto dir_name = OsString{ full_path_buf.data(), file_name_ptr };
	auto full_path = OsString{ full_path_buf.data() };

	return FileSystemApi::SearchFileResult{ std::move(dir_name), std::move(full_path) };
}

// カレントディレクトリを基準としてファイルを探し、なければ指定された各ディレクトリから探す。
template<typename TDirs>
static auto search_file_from_dirs(
	OsStringView const& file_ref,
	TDirs const& dirs,
	FileSystemApi& api
) -> std::optional<FileSystemApi::SearchFileResult> {
	{
		auto result_opt = api.search_file_from_current_dir(file_ref);
		if (result_opt) {
			return result_opt;
		}
	}

	for (auto&& dir : dirs) {
		auto result_opt = api.search_file_from_dir(file_ref, dir);
		if (result_opt) {
			return result_opt;
		}
	}

	return std::nullopt;
}

auto WindowsFileSystemApi::read_all_text(OsString const& file_path)->std::optional<std::string> {
	return ::read_all_text(file_path);
}

auto WindowsFileSystemApi::search_file_from_dir(OsStringView file_name, OsStringView base_dir)->std::optional<SearchFileResult> {
	return ::search_file_from_dir(file_name, base_dir);
}

auto WindowsFileSystemApi::search_file_from_current_dir(OsStringView file_name)->std::optional<SearchFileResult> {
	return ::search_file_from_dir(file_name, std::nullopt);
}

// -----------------------------------------------
// hsptmp の解決
// -----------------------------------------------

// hsptmp に対応するファイル参照名を見つける。
//
// 解説:
// スクリプトエディタで未保存の内容が、カレントディレクトリの hsptmp に保存されているはず。
// 1. タブがファイルを開いているのではなく「無題」のとき、
//    hsptmp に相当するファイル名は ??? (ファイル名なし) になっている。
// 2. タブがファイルを開いているとき、hsptmp は「最初に出現する」ファイルに対応するはず。
//    ただし、実行されたスクリプトより前に標準ヘッダファイル (hspdef.as など) が読まれるので、
//    正確には「最後に hspdef.as が出現する行の次の行のファイル名」を見つけて hsptmp とみなす。
// (この処理は HSP のコンパイラの実装に依存したものである。)
static auto resolve_hsptmp(std::vector<std::string> const& file_ref_names) -> std::optional<std::string> {
	// 逆順にループを回して hspdef.as が最後に出現する位置を探し、あればその次を返す。
	for (auto i = file_ref_names.size(); i >= 1;) {
		i--;

		if (file_ref_names[i] == "hspdef.as" && i + 1 < file_ref_names.size()) {
			return file_ref_names[i + 1];
		}
	}

	return std::nullopt;
}

// -----------------------------------------------
// SourceFileResolver
// -----------------------------------------------

void SourceFileResolver::add_known_dir(OsString&& dir) {
	dirs_.emplace(std::move(dir));
}

void SourceFileResolver::add_file_ref_name(std::string&& file_ref_name) {
	file_ref_names_.push_back(std::move(file_ref_name));
}

void SourceFileResolver::dedup() {
	// 注意: 解決処理の精度を上げるため、ファイル参照名の順番を維持する。

	auto output = std::vector<std::string>{};
	auto done = std::unordered_set<std::string>{};

	for (auto&& file_ref_name : file_ref_names_) {
		auto pair = done.insert(file_ref_name);
		auto added = pair.second;
		if (added) {
			output.push_back(file_ref_name);
		}
	}

	file_ref_names_ = std::move(output);
}

auto SourceFileResolver::resolve() -> SourceFileRepository {
	// 未解決のファイル参照名の集合
	auto file_ref_names = std::vector<std::pair<std::string, OsString>>{};

	// 絶対パス → ファイルID
	auto full_path_map = std::unordered_map<OsString, SourceFileId>{};

	// ファイルID → ソースファイル
	auto source_files = std::vector<SourceFile>{};

	// ファイル参照名 → 絶対パス
	auto full_paths = std::unordered_map<std::string, OsString>{};

	// ファイル参照名と絶対パスのペアを登録する。
	auto add = [&](std::string const& file_ref_name, OsString const& full_path) {
		assert(!full_paths.count(file_ref_name));
		full_paths.emplace(file_ref_name, full_path);

		// 対応するソースファイルがなければ追加する。
		auto iter = full_path_map.find(full_path);
		if (iter == full_path_map.end()) {
			auto file_id = SourceFileId{ source_files.size() };
			full_path_map.emplace(full_path, file_id);
			source_files.emplace_back(to_owned(full_path), fs_);
		}
	};

	// ファイル参照名の解決を試みる。
	auto find = [&](std::string const& original, OsString const& file_ref_name) -> bool {
		assert(!full_path_map.count(file_ref_name) && u8"解決済みのファイルには呼ばれないはず");

		// ファイルシステムから探す。
		auto result_opt = search_file_from_dirs(file_ref_name, dirs_, fs_);
		if (!result_opt) {
			return false;
		}

		// 見つかったディレクトリを検索対象に加える。
		dirs_.emplace(std::move(result_opt->dir_path_));

		// メモ化する。
		add(original, result_opt->full_path_);
		return true;
	};

	// hsptmp に対応するファイルを見つける。
	// hsptmp の絶対パスを見つけておくため、ファイル参照名に hsptmp を追加する。
	auto hsptmp_ref_name_opt = resolve_hsptmp(file_ref_names_);
	add_file_ref_name("hsptmp");

	// ファイル参照名の重複を除去する。ついでに OsString に変換しておく。
	dedup();
	for (auto&& file_ref_name : file_ref_names_) {
		auto os_str = to_os(as_hsp(file_ref_name));
		file_ref_names.emplace_back(file_ref_name, std::move(os_str));
	}

	while (true) {
		// ループ中に何か変化が起こったら false。
		// ループの最後まで true なら、もう何も起きないのでループ終了。
		auto stuck = true;

		// 要素を削除する必要があるので後ろから前にループを回す。
		for (auto i = file_ref_names.size(); i >= 1;) {
			i--;

			auto ok = find(file_ref_names[i].first, file_ref_names[i].second);
			if (!ok) {
				continue;
			}

			// i 番目の要素を削除
			std::swap(file_ref_names[i], file_ref_names.back());
			file_ref_names.pop_back();

			stuck = false;
		}

		if (stuck) {
			break;
		}
	}

	// ファイル参照名 → ファイルID
	auto file_map = std::unordered_map<std::string, SourceFileId>{};
	for (auto&& pair : full_paths) {
		auto iter = full_path_map.find(pair.second);
		if (iter == full_path_map.end()) {
			assert(false);
			continue;
		}

		file_map.emplace(pair.first, iter->second);
	}

	// hsptmp を解決する。
	if (hsptmp_ref_name_opt) {
		auto hsptmp_iter = file_map.find("hsptmp");
		if (hsptmp_iter != file_map.end()) {
			auto hsptmp_full_path = source_files[hsptmp_iter->second.id()].full_path();

			auto file_iter = file_map.find(*hsptmp_ref_name_opt);
			if (file_iter != file_map.end()) {
				source_files[file_iter->second.id()].set_content_file_path(OsString{ hsptmp_full_path });
			}
		}
	}

	return SourceFileRepository{ std::move(source_files), std::move(file_map) };
}

// -----------------------------------------------
// SourceFileRepository
// -----------------------------------------------

auto SourceFileRepository::file_ref_name_to_file_id(char const* file_ref_name) const->std::optional<SourceFileId> {
	auto iter = file_map_.find(file_ref_name);
	if (iter == file_map_.end()) {
		return std::nullopt;
	}

	return iter->second;
}

auto SourceFileRepository::file_ref_name_to_full_path(char const* file_ref_name) const->std::optional<OsStringView> {
	auto file_id_opt = file_ref_name_to_file_id(file_ref_name);
	if (!file_id_opt) {
		return std::nullopt;
	}

	return file_to_full_path(*file_id_opt);
}

auto SourceFileRepository::file_ref_name_to_content(char const* file_ref_name)->std::optional<std::u8string_view> {
	auto file_id_opt = file_ref_name_to_file_id(file_ref_name);
	if (!file_id_opt) {
		return std::nullopt;
	}

	return file_to_content(*file_id_opt);
}

auto SourceFileRepository::file_ref_name_to_line_at(char const* file_ref_name, std::size_t line_index)->std::optional<std::u8string_view> {
	auto file_id_opt = file_ref_name_to_file_id(file_ref_name);
	if (!file_id_opt) {
		return std::nullopt;
	}

	return file_to_line_at(*file_id_opt, line_index);
}

auto SourceFileRepository::file_to_full_path(SourceFileId const& file_id) const->std::optional<OsStringView> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return std::make_optional(source_files_[file_id.id()].full_path());
}

auto SourceFileRepository::file_to_full_path_as_utf8(SourceFileId const& file_id) const->std::optional<std::u8string_view> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return std::make_optional(source_files_[file_id.id()].full_path_as_utf8());
}

auto SourceFileRepository::file_to_content(SourceFileId const& file_id) -> std::optional<std::u8string_view> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return std::make_optional(source_files_[file_id.id()].content());
}

auto SourceFileRepository::file_to_line_at(SourceFileId const& file_id, std::size_t line_index) -> std::optional<std::u8string_view> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return source_files_[file_id.id()].line_at(line_index);
}

static auto load_text_file(OsString const& file_path, FileSystemApi& fs) -> std::u8string {
	auto content_opt = fs.read_all_text(file_path);
	if (!content_opt) {
		// デバッグログなどに出力する？
		return to_owned(u8"");
	}

	// NOTE: ソースコードの文字コードが HSP ランタイムの文字コードと同じとは限らない。
	return to_utf8(as_hsp(std::move(*content_opt)));
}

static auto char_is_whitespace(char8_t c) -> bool {
	return c == u8' ' || c == u8'\t';
}

// 行ごとに分割する。ただし各行の字下げは除外する。
static auto split_by_lines(std::u8string_view const& str) -> std::vector<std::u8string_view> {
	auto lines = std::vector<std::u8string_view>{};

	for (auto&& line : StringLines{ str }) {
		auto i = std::size_t{};
		while (i < line.size() && char_is_whitespace(line[i])) {
			i++;
		}

		lines.emplace_back(line.substr(i));
	}

	return lines;
}

// -----------------------------------------------
// SourceFile
// -----------------------------------------------

SourceFile::SourceFile(OsString&& full_path, FileSystemApi& fs)
	: full_path_(std::move(full_path))
	, full_path_as_utf8_(to_utf8(full_path_))
	, loaded_(false)
	, content_()
	, lines_()
	, fs_(fs)
{
}

auto SourceFile::content() -> std::u8string_view {
	load();

	return content_;
}

auto SourceFile::line_at(std::size_t line_index) -> std::optional<std::u8string_view> {
	load();

	if (line_index >= lines_.size()) {
		return std::nullopt;
	}

	return std::make_optional(lines_[line_index]);
}

void SourceFile::load() {
	if (loaded_) {
		return;
	}

	auto full_path = content_file_path_.value_or(full_path_);

	content_ = normalize_lines(load_text_file(full_path, fs_));
	lines_ = split_by_lines(content_);
	loaded_ = true;
}

// -----------------------------------------------
// テスト
// -----------------------------------------------

template<typename Char>
static auto char_is_path_sep(Char c) -> bool {
	return c == (Char)'/' || c == (Char)'\\';
}

// knowbug_config に同様のコードがあるのでまとめる
static void path_drop_file_name(OsString& full_path) {
	while (!full_path.empty()) {
		auto last = full_path[full_path.length() - 1];
		if (char_is_path_sep(last)) {
			full_path.pop_back();
			break;
		}

		full_path.pop_back();
	}
}

class VirtualFileSystemApi
	: public FileSystemApi
{
	std::unordered_map<OsString, std::string> files_;

	OsString current_dir_;

public:
	void set_current_dir(OsString current_dir) {
		current_dir_ = std::move(current_dir);
	}

	void add_file(OsString file_path, std::string content) {
		files_[std::move(file_path)] = std::move(content);
	}

	auto read_all_text(OsString const& file_path)->std::optional<std::string> override {
		auto iter = files_.find(file_path);
		return iter != files_.end() ? std::make_optional(iter->second) : std::nullopt;
	}

	auto search_file_from_dir(OsStringView file_name, OsStringView base_dir)->std::optional<SearchFileResult> override {
		auto starts_with = [](OsStringView str, OsStringView prefix) -> bool {
			return str.size() >= prefix.size() && str.substr(0, prefix.size()) == prefix;
		};

		auto file_path = to_owned(base_dir);
		file_path += TEXT("/");
		file_path += file_name;

		for (auto&& pair : files_) {
			if (pair.first == file_path) {
				auto dir_name = to_owned(file_path);
				path_drop_file_name(dir_name);

				return SearchFileResult{ dir_name, file_path };
			}
		}

		return std::nullopt;
	}

	auto search_file_from_current_dir(OsStringView file_name)->std::optional<SearchFileResult> override {
		return search_file_from_dir(file_name, current_dir_);
	}
};

static auto create_file_system_for_testing() -> VirtualFileSystemApi {
	auto fs = VirtualFileSystemApi{};
	fs.set_current_dir(TEXT("/src"));

	fs.add_file(TEXT("/hsp/common/hspdef.as"), "// hsp_def");
	fs.add_file(TEXT("/hsp/common/awesome_plugin/awesome_plugin.hsp"), "// awesome_plugin");

	fs.add_file(TEXT("/src/main.hsp"), "// main");
	fs.add_file(TEXT("/src/awesome_lib/awesome_lib.hsp"), "// awesome_lib");

	fs.add_file(TEXT("/src/not_included.txt"), "// not included");
	return fs;
}

void source_files_tests(Tests& tests) {
	auto& suite = tests.suite(u8"source_files");

	suite.test(
		u8"ソースファイルを解決できる",
		[&](TestCaseContext& t) {
			auto fs = create_file_system_for_testing();

			auto resolver = SourceFileResolver{ fs };
			resolver.add_known_dir(to_owned(TEXT("/hsp/common")));

			resolver.add_file_ref_name("hspdef.as");
			resolver.add_file_ref_name("main.hsp");
			resolver.add_file_ref_name("awesome_plugin/awesome_plugin.hsp");
			resolver.add_file_ref_name("awesome_lib/awesome_lib.hsp");

			auto repository = resolver.resolve();

			if (!t.eq(repository.file_count(), 4)) {
				return false;
			}

			if (!t.eq(repository.file_ref_name_to_file_id("unknown_file_ref_name").has_value(), false)) {
				return false;
			}

			return t.eq(to_utf8(*repository.file_ref_name_to_full_path("main.hsp")), u8"/src/main.hsp");
		});

	suite.test(
		u8"ソースファイルの内容を取得できる",
		[&](TestCaseContext& t) {
			auto content =
				"  main\r\n"
				"  stop\r\n";

			auto fs = create_file_system_for_testing();
			fs.add_file(TEXT("/src/main.hsp"), content);

			auto resolver = SourceFileResolver{ fs };
			resolver.add_file_ref_name("main.hsp");

			auto repository = resolver.resolve();

			auto const& file_id = *repository.file_ref_name_to_file_id("main.hsp");

			return t.eq(as_native(*repository.file_to_content(file_id)), content)
				&& t.eq(as_native(*repository.file_to_line_at(file_id, 0)), "main")
				&& t.eq(as_native(*repository.file_to_line_at(file_id, 1)), "stop")
				&& t.eq(as_native(repository.file_to_line_at(file_id, 9999).value_or(as_utf8(u8""))), "");
		});

	suite.test(
		u8"実行スクリプトファイルの内容を hsptmp から読む",
		[&](TestCaseContext& t) {
			auto fs = create_file_system_for_testing();

			fs.add_file(TEXT("/src/main.hsp"), "saved script");
			fs.add_file(TEXT("/src/hsptmp"), "run script");
			fs.add_file(TEXT("/hsp/common/hsptmp"), "should not be found"); // common の hsptmp は探さないでほしい

			auto resolver = SourceFileResolver{ fs };
			resolver.add_known_dir(to_owned(TEXT("/hsp/common")));
			resolver.add_file_ref_name("hspdef.as");
			resolver.add_file_ref_name("userdef.as");
			resolver.add_file_ref_name("hspdef.as");
			resolver.add_file_ref_name("main.hsp");
			resolver.add_file_ref_name("other.hsp");

			auto repository = resolver.resolve();
			auto const& file_id = *repository.file_ref_name_to_file_id("main.hsp");

			return t.eq(as_native(*repository.file_to_content(file_id)), "run script")
				&& t.eq(*repository.file_to_full_path(file_id), TEXT("/src/main.hsp"));
		});
}
