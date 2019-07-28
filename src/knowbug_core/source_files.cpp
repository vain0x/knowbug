#include "pch.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include "source_files.h"
#include "string_split.h"

// 指定したディレクトリを基準として、指定した名前または相対パスのファイルを検索する。
// 結果として、フルパスと、パス中のディレクトリの部分を返す。
// use_current_dir=true のときは、指定したディレクトリではなく、カレントディレクトリで検索する。
static auto search_file_from_dir(
	OsStringView const& file_ref,
	OsStringView const& base_dir,
	bool use_current_dir,
	OsString& out_dir_name,
	OsString& out_full_path
) -> bool {
	auto file_name_ptr = LPTSTR{};
	auto full_path_buf = std::array<TCHAR, MAX_PATH>{};

	auto base_dir_ptr = use_current_dir ? nullptr : base_dir.data();
	auto succeeded =
		SearchPath(
			base_dir_ptr, file_ref.data(), /* lpExtenson = */ nullptr,
			full_path_buf.size(), full_path_buf.data(), &file_name_ptr
		) != 0;
	if (!succeeded) {
		return false;
	}

	assert(full_path_buf.data() <= file_name_ptr && file_name_ptr <= full_path_buf.data() + full_path_buf.size());
	auto dir_name = OsString{ full_path_buf.data(), file_name_ptr };

	out_dir_name = std::move(dir_name);
	out_full_path = OsString{ full_path_buf.data() };
	return true;
}

// 複数のディレクトリを基準としてファイルを探し、カレントディレクトリからも探す。
template<typename TDirs>
static auto search_file_from_dirs(
	OsStringView const& file_ref,
	TDirs const& dirs,
	OsString& out_dir_name,
	OsString& out_full_path
) -> bool {
	for (auto&& dir : dirs) {
		auto ok = search_file_from_dir(
			file_ref, as_view(dir), /* use_current_dir = */ false,
			out_dir_name, out_full_path
		);
		if (!ok) {
			continue;
		}

		return true;
	}

	// カレントディレクトリから探す。
	auto no_dir = as_os(TEXT(""));
	return search_file_from_dir(
		file_ref, no_dir, true,
		out_dir_name, out_full_path
	);
}

// -----------------------------------------------
// SourceFileResolver
// -----------------------------------------------

void SourceFileResolver::add_known_dir(OsString&& dir) {
	dirs_.emplace(std::move(dir));
}

void SourceFileResolver::add_file_ref_name(std::string&& file_ref_name) {
	file_ref_names_.emplace(std::move(file_ref_name));
}

auto SourceFileResolver::resolve() -> SourceFileRepository {
	// 未解決のファイル参照名の集合
	auto file_ref_names = std::vector<std::pair<std::string, OsString>>{};

	for (auto&& file_ref_name : std::move(file_ref_names_)) {
		auto os_str = to_os(as_hsp(file_ref_name));
		file_ref_names.emplace_back(std::move(file_ref_name), std::move(os_str));
	}

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
		auto&& iter = full_path_map.find(full_path);
		if (iter == full_path_map.end()) {
			auto file_id = SourceFileId{ source_files.size() };
			full_path_map.emplace(full_path, file_id);
			source_files.emplace_back(to_owned(full_path));
		}
	};

	// ファイル参照名の解決を試みる。
	auto find = [&](std::string const& original, OsString const& file_ref_name) -> bool {
		assert(!full_path_map.count(file_ref_name) && u8"解決済みのファイルには呼ばれないはず");

		// ファイルシステムから探す。
		OsString dir_name;
		OsString full_path;
		auto ok = search_file_from_dirs(file_ref_name, dirs_, dir_name, full_path);
		if (!ok) {
			return false;
		}

		// 見つかったディレクトリを検索対象に加える。
		dirs_.emplace(std::move(dir_name));

		// メモ化する。
		add(original, full_path);
		return true;
	};

	while (true) {
		// ループ中に何か変化が起こったら false。
		// ループの最後まで true なら、もう何も起きないのでループ終了。
		auto stuck = true;

		// 要素を削除する必要があるので後ろから前にループを回す。
		auto i = file_ref_names.size();
		while (i > 0) {
			i--;

			auto ok = find(file_ref_names[i].first, file_ref_names[i].second);
			if (!ok) {
				continue;
			}

			// i 番目の要素を削除
			std::iter_swap(&file_ref_names[i], &file_ref_names.back());
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
		auto&& iter = full_path_map.find(pair.second);
		if (iter == full_path_map.end()) {
			assert(false);
			continue;
		}

		file_map.emplace(pair.first, iter->second);
	}

	return SourceFileRepository{ std::move(source_files), std::move(file_map) };
}

// -----------------------------------------------
// SourceFileRepository
// -----------------------------------------------

auto SourceFileRepository::file_ref_name_to_file_id(char const* file_ref_name) const->std::optional<SourceFileId> {
	auto&& iter = file_map_.find(file_ref_name);
	return std::make_optional(iter->second);
}

auto SourceFileRepository::file_ref_name_to_full_path(char const* file_ref_name) const->std::optional<OsStringView> {
	auto file_id_opt = file_ref_name_to_file_id(file_ref_name);
	if (!file_id_opt) {
		return std::nullopt;
	}

	return file_to_full_path(*file_id_opt);
}

auto SourceFileRepository::file_ref_name_to_content(char const* file_ref_name)->std::optional<Utf8StringView> {
	auto file_id_opt = file_ref_name_to_file_id(file_ref_name);
	if (!file_id_opt) {
		return std::nullopt;
	}

	return file_to_content(*file_id_opt);
}

auto SourceFileRepository::file_ref_name_to_line_at(char const* file_ref_name, std::size_t line_index)->std::optional<Utf8StringView> {
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

auto SourceFileRepository::file_to_full_path_as_utf8(SourceFileId const& file_id) const->std::optional<Utf8StringView> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return std::make_optional(source_files_[file_id.id()].full_path_as_utf8());
}

auto SourceFileRepository::file_to_content(SourceFileId const& file_id) -> std::optional<Utf8StringView> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return std::make_optional(source_files_[file_id.id()].content());
}

auto SourceFileRepository::file_to_line_at(SourceFileId const& file_id, std::size_t line_index) -> std::optional<Utf8StringView> {
	if (file_id.id() >= source_files_.size()) {
		assert(false && u8"unknown source file id");
		return std::nullopt;
	}

	return source_files_[file_id.id()].line_at(line_index);
}


// NOTE: ifstream がファイル名に basic_string_view を受け取らないので、OsString への参照をもらう。
static auto load_text_file(OsString const& file_path) -> Utf8String {
	auto ifs = std::ifstream{ file_path };
	if (!ifs.is_open()) {
		// デバッグログなどに出力する？
		return to_owned(as_utf8(u8""));
	}

	// NOTE: ソースコードの文字コードが HSP ランタイムの文字コードと同じとは限らない。
	auto content = as_hsp(std::string{ std::istreambuf_iterator<char>{ ifs }, {} });
	return to_utf8(content);
}

static auto char_is_whitespace(Utf8Char c) -> bool {
	return c == Utf8Char{ ' ' } || c == Utf8Char{ '\t' };
}

// 行ごとに分割する。ただし各行の字下げは除外する。
static auto split_by_lines(Utf8StringView const& str) -> std::vector<Utf8StringView> {
	auto lines = std::vector<Utf8StringView>{};

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

SourceFile::SourceFile(OsString&& full_path)
	: full_path_(std::move(full_path))
	, full_path_as_utf8_(to_utf8(full_path_))
	, loaded_(false)
	, content_()
	, lines_()
{
}

auto SourceFile::content() -> Utf8StringView {
	load();

	return content_;
}

auto SourceFile::line_at(std::size_t line_index) -> std::optional<Utf8StringView> {
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

	content_ = load_text_file(full_path_);
	lines_ = split_by_lines(as_view(content_));
	loaded_ = true;
}