#include <unordered_set>
#include <tchar.h>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "module/LineDelimitedString.h"

#include "module/supio/supio.h"

struct VTNodeScript::Impl
{
	bool resolutionDone_;

	std::unordered_set<OsString> userDirs_;
	unordered_map<OsString, shared_ptr<OsString>> fullPathFromRefName_;
	unordered_map<OsString, LineDelimitedString> cache_;

public:
	// 指定されたファイル名の絶対パスを検索する。
	auto searchFile(OsStringView fileName)->shared_ptr<OsString>;

	// 指定されたファイル名の絶対パスを、指定したディレクトリを基準にして検索する。
	// ただし no_dir = true のときは dir 引数は無視して、カレントディレクトリから検索する。
	auto searchFile(OsStringView fileName, OsStringView dir, bool no_dir)->shared_ptr<OsString>;

	auto fetchScript(char const* fileName)->optional_ref<LineDelimitedString>;
};

VTNodeScript::VTNodeScript()
	: p_(new Impl{})
{
	p_->resolutionDone_ = false;
}

VTNodeScript::~VTNodeScript()
{}

auto VTNodeScript::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

auto VTNodeScript::Impl::searchFile(OsStringView fileRefName, OsStringView dir, bool no_dir)
-> shared_ptr<OsString>
{
	auto file_name_ptr = LPTSTR{};
	auto full_path_buf = std::array<TCHAR, MAX_PATH>{};

	auto dir_ptr = no_dir ? nullptr : dir.data();
	auto succeeded =
		SearchPath(
			dir_ptr, fileRefName.data(), /* lpExtenson = */ nullptr,
			full_path_buf.size(), full_path_buf.data(), &file_name_ptr
		) != 0;
	if (!succeeded) {
		return nullptr;
	}

	// 発見されたディレクトリを検索対象に追加する。
	assert(full_path_buf.data() <= file_name_ptr && file_name_ptr <= full_path_buf.data() + full_path_buf.size());
	auto dir_name = OsString::from_range(full_path_buf.data(), file_name_ptr);

	userDirs_.emplace(std::move(dir_name));

	auto p = std::make_shared<OsString>(full_path_buf.data());

	// メモ化
	fullPathFromRefName_[fileRefName.to_owned()] = p; // FIXME: 無駄なコピー

	return p;
}

auto VTNodeScript::Impl::searchFile(OsStringView fileRefName) -> shared_ptr<OsString> {
	// メモから読む
	auto iter = fullPathFromRefName_.find(fileRefName.to_owned()); // FIXME: 無駄なコピー
	if (iter != fullPathFromRefName_.end()) {
		return iter->second;
	}

	// ユーザディレクトリ、カレントディレクトリ、common、の順で探す
	for (auto const& dir : userDirs_) {
		if (auto p = searchFile(fileRefName, dir.as_ref(), true)) {
			return std::move(p);
		}
	}
	if (auto p = searchFile(fileRefName, OsStringView{ TEXT("") }, false)) {
		return std::move(p);
	}
	return searchFile(fileRefName, g_config->commonPath().as_ref(), true);
}

auto VTNodeScript::resolveRefName(char const* fileRefNameInput) const
-> shared_ptr<string const>
{
	auto fileRefNameBuf = HspStringView{ fileRefNameInput }.to_os_string();
	auto fileRefName = fileRefNameBuf.as_ref();

	if (auto p = p_->searchFile(fileRefName)) {
		return std::make_shared<string const>(string{ p->to_hsp_string().data() }); // FIXME: 無駄なコピー
	}

	while (!p_->resolutionDone_) {
		bool stuck = true;

		for (auto&& refName : hpiutil::fileRefNames()) {
			auto ref_name_str = HspStringView{ refName.data() }.to_os_string(); // FIXME: 無駄なコピー

			if (p_->fullPathFromRefName_.count(ref_name_str) != 0) continue;
			if (auto p = p_->searchFile(ref_name_str.as_ref())) {
				stuck = false;
				if (ref_name_str.as_ref() == fileRefName) {
					return std::make_shared<string const>(string{ p->to_hsp_string().data() }); // FIXME: 無駄なコピー
				}
			}
		}
		if (stuck) { p_->resolutionDone_ = true; }
	}

	auto p = p_->fullPathFromRefName_[fileRefName.to_owned()]; // FIXME: 無駄なコピー
	return std::make_shared<string const>(string{ p->to_hsp_string().data() }); // FIXME: 無駄なコピー
}

auto VTNodeScript::Impl::fetchScript(char const* fileRefName)
-> optional_ref<LineDelimitedString>
{
	if (auto p = VTRoot::script().resolveRefName(fileRefName)) {
		auto filePath = HspStringView{ p->data() }.to_os_string(); // FIXME: 無駄なコピー

		auto& lds = map_find_or_insert(cache_, filePath, [&filePath]() {
			auto ifs = std::ifstream{ filePath.data() };
			assert(ifs.is_open());
			return LineDelimitedString(ifs);
		});
		return &lds;
	}
	else {
		return nullptr;
	}
}

auto VTNodeScript::fetchScriptAll(char const* fileRefName) const
-> optional_ref<string const>
{
	if (auto p = p_->fetchScript(fileRefName)) {
		return &p->get();
	}
	else {
		return nullptr;
	}
}

auto VTNodeScript::fetchScriptLine(hpiutil::SourcePos const& spos) const
-> unique_ptr<string const>
{
	if (auto p = p_->fetchScript(spos.fileRefName())) {
		/**
		編集中のファイルが実行されている場合、ファイルの内容が実際と異なることがある。
		行番号のアウトレンジに注意。
		//*/
		return std::make_unique<string>(p->line(spos.line()));
	}
	else {
		return nullptr;
	}
}
