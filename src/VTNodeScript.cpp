
#include <unordered_set>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "module/LineDelimitedString.h"

struct VTNodeScript::Impl
{
	bool resolutionDone_;

	std::unordered_set<string> userDirs_;
	unordered_map<string, shared_ptr<string const>> fullPathFromRefName_;
	unordered_map<string, LineDelimitedString> cache_;

public:
	auto searchFile(string const& fileName)->shared_ptr<string const>;
	auto searchFile(string const& fileName, char const* dir)->shared_ptr<string const>;
	auto fetchScript(char const* fileName) -> optional_ref<LineDelimitedString>;
};

VTNodeScript::VTNodeScript()
	: p_(new Impl {})
{
	p_->resolutionDone_ = false;
}

VTNodeScript::~VTNodeScript()
{}

auto VTNodeScript::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

auto VTNodeScript::Impl::searchFile(string const& fileRefName, char const* dir)
	-> shared_ptr<string const>
{
	auto fileName = static_cast<char*>(nullptr);
	auto fullPath = std::array<char, MAX_PATH> {};
	auto succeeded =
		SearchPath
			( dir, fileRefName.c_str(), /* lpExtenson = */ nullptr
			, fullPath.size(), fullPath.data(), &fileName)
		!= 0;
	if ( succeeded ) {
		// 発見されたディレクトリを検索対象に追加する
		userDirs_.emplace(string(fullPath.data(), fileName));

		auto p = std::make_shared<string const>(fullPath.data());

		// メモ化
		fullPathFromRefName_.emplace(fileRefName, p);
		return p;
	} else {
		return nullptr;
	}
}

auto VTNodeScript::Impl::searchFile(string const& fileRefName)
	-> shared_ptr<string const>
{
	// メモから読む
	auto iter = fullPathFromRefName_.find(fileRefName);
	if ( iter != fullPathFromRefName_.end() ) {
		return iter->second;
	}

	// ユーザディレクトリ、カレントディレクトリ、common、の順で探す
	for ( auto const& dir : userDirs_ ) {
		if ( auto p = searchFile(fileRefName, dir.c_str()) ) {
			return std::move(p);
		}
	}
	if ( auto p = searchFile(fileRefName, nullptr) ) {
		return std::move(p);
	}
	return searchFile(fileRefName, g_config->commonPath().c_str());
}

auto VTNodeScript::resolveRefName(string const& fileRefName) const
	-> shared_ptr<string const>
{
	if ( auto p = p_->searchFile(fileRefName) ) {
		return p;
	}

	while ( ! p_->resolutionDone_ ) {
		bool stuck = true;

		for ( auto&& refName : hpiutil::fileRefNames() ) {
			if ( p_->fullPathFromRefName_.count(refName) != 0 ) continue;
			if ( auto p = p_->searchFile(refName) ) {
				stuck = false;
				if ( refName == fileRefName ) { return p; }
			}
		}
		if ( stuck ) { p_->resolutionDone_ = true; }
	}
	return p_->fullPathFromRefName_[fileRefName];
}

auto VTNodeScript::Impl::fetchScript(char const* fileRefName)
	-> optional_ref<LineDelimitedString>
{
	if ( auto p = VTRoot::script().resolveRefName(fileRefName) ) {
		auto const& filePath = *p;

		auto& lds = map_find_or_insert(cache_, filePath, [&filePath] () {
			auto ifs = std::ifstream { filePath };
			assert(ifs.is_open());
			return LineDelimitedString(ifs);
		});
		return &lds;
	} else {
		return nullptr;
	}
}

auto VTNodeScript::fetchScriptAll(char const* fileRefName) const
	-> optional_ref<string const>
{
	if ( auto p = p_->fetchScript(fileRefName) ) {
		return &p->get();
	} else {
		return nullptr;
	}
}

auto VTNodeScript::fetchScriptLine(char const* fileRefName, size_t lineIndex) const
	-> unique_ptr<string const>
{
	if ( auto p = p_->fetchScript(fileRefName) ) {
		/**
		編集中のファイルが実行されている場合、ファイルの内容が実際と異なることがある。
		行番号のアウトレンジに注意。
		//*/
		return std::make_unique<string>(p->line(lineIndex));
	} else {
		return nullptr;
	}
}
