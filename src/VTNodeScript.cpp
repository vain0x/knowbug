
#include <set>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "module/LineDelimitedString.h"

struct VTNodeScript::Impl
{
public:
	auto searchFile(char const* fileName, char const* dir) -> unique_ptr<string const>;
};

VTNodeScript::VTNodeScript()
	: p_(new Impl {})
{}

auto VTNodeScript::parent() const -> shared_ptr<VTNodeData>
{
	return VTRoot::make_shared();
}

auto VTNodeScript::Impl::searchFile(char const* fileRefName, char const* dir)
	-> unique_ptr<string const>
{
	char* fileName = nullptr;
	std::array<char, MAX_PATH> fullPath {};
	bool const succeeded =
		SearchPath
			( dir, fileRefName, /* lpExtenson = */ nullptr
			, fullPath.size(), fullPath.data(), &fileName)
		!= 0;
	if ( succeeded ) {
		return std::make_unique<string>(fullPath.data());
	} else {
		return nullptr;
	}
}

auto VTNodeScript::searchFile(char const* fileRefName) const -> unique_ptr<string const>
{
	if ( auto&& p = p_->searchFile(fileRefName, nullptr) ) {
		return std::move(p);
	}
	return p_->searchFile(fileRefName, g_config->commonPath().c_str());
}
