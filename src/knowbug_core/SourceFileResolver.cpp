#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include "../hpiutil/hpiutil.hpp"
#include "../hpiutil/DInfo.hpp"
#include "SourceFileResolver.h"

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

static auto open_source_file(OsStringView const& full_path) -> std::shared_ptr<SourceFile> {
	auto ifs = std::ifstream{ full_path.data() };
	assert(ifs.is_open());

	// FIXME: ソースコードの文字コードが HSP ランタイムの文字コードと同じとは限らない。
	auto content = std::string{ std::istreambuf_iterator<char>{ifs}, {} };
	auto content_str = to_os(as_hsp(std::move(content)));

	return std::make_shared<SourceFile>(to_owned(full_path), std::move(content_str));
}

SourceFileResolver::SourceFileResolver(OsString&& common_path, hpiutil::DInfo const& debug_segment)
	: resolution_done_(false)
	, debug_segment_(debug_segment)
{
	dirs_.emplace(std::move(common_path));
}

auto SourceFileResolver::resolve_file_ref_names()->void {
	if (resolution_done_) {
		return;
	}

	auto file_ref_names = std::vector<OsString>{};
	for (auto&& file_ref_name : debug_segment_.fileRefNames()) {
		file_ref_names.push_back(to_os(as_hsp(file_ref_name.data())));
	}

	while (!resolution_done_) {
		auto stuck = true;

		for (auto&& file_ref_name : file_ref_names) {
			// 絶対パス発見済みならOK。
			if (full_paths_.count(file_ref_name) != 0) {
				continue;
			}

			// 絶対パスを探す。
			auto&& full_path_opt = find_full_path_core(as_view(file_ref_name));
			if (!full_path_opt) {
				continue;
			}

			stuck = false;
		}

		// ループを回して何も起こらなくなったら (不動点に達したら) 終わり。
		if (stuck) {
			resolution_done_ = true;
		}
	}
}

auto SourceFileResolver::find_full_path(OsStringView const& file_ref_name) -> std::optional<OsStringView> {
	// 依存関係の解決をする。
	resolve_file_ref_names();

	// 探す。
	return find_full_path_core(file_ref_name);
}

auto SourceFileResolver::find_full_path_core(OsStringView const& file_ref_name) -> std::optional<OsStringView> {
	auto file_ref_name_str = to_owned(file_ref_name); // FIXME: 無駄なコピー

	// キャッシュから探す。
	auto iter = full_paths_.find(file_ref_name_str);
	if (iter != std::end(full_paths_)) {
		return std::make_optional(as_view(iter->second));
	}

	// ファイルシステムから探す。
	OsString dir_name;
	OsString full_path;
	auto ok = search_file_from_dirs(file_ref_name, dirs_, dir_name, full_path);
	if (ok) {
		// 見つかったディレクトリを検索対象に加える。
		dirs_.emplace(std::move(dir_name));

		// メモ化する。
		full_paths_.emplace(to_owned(file_ref_name), std::move(full_path));

		// メモ内の絶対パスへの参照を返す。
		return std::make_optional(as_view(full_paths_[file_ref_name_str]));
	}

	return std::nullopt;
}

auto SourceFileResolver::find_script_content(OsStringView const& file_ref_name) -> std::optional<OsStringView> {
	auto&& source_file_opt = find_source_file(file_ref_name);
	if (!source_file_opt) {
		return std::nullopt;
	}

	auto&& content = (*source_file_opt)->content();
	return std::make_optional(content);
}

auto SourceFileResolver::find_script_line(OsStringView const& file_ref_name, std::size_t line_index)->std::optional<OsStringView> {
	auto&& source_file_opt = find_source_file(file_ref_name);
	if (!source_file_opt) {
		return std::nullopt;
	}

	auto&& content  = (*source_file_opt)->line_at(line_index);
	return std::make_optional(content);
}

auto SourceFileResolver::find_source_file(OsStringView const& file_ref_name) -> std::optional<std::shared_ptr<SourceFile>> {
	auto&& full_path_opt = find_full_path(file_ref_name);
	if (!full_path_opt) {
		return std::nullopt;
	}

	auto source_file = open_source_file(*full_path_opt);
	source_files_.emplace(to_owned(*full_path_opt), std::move(source_file));

	return std::make_optional(source_files_[to_owned(*full_path_opt)]); // FIXME: 無駄なコピー
}
