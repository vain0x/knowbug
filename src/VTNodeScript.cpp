#include <unordered_set>
#include <tchar.h>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "SourceFileResolver.h"

struct VTNodeScript::Impl
{
	SourceFileResolver resolver_;

public:
	Impl()
		: resolver_(g_config->commonPath())
	{
	}
};

VTNodeScript::VTNodeScript()
	: p_(new Impl{})
{
}

VTNodeScript::~VTNodeScript()
{}

auto VTNodeScript::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

auto VTNodeScript::resolveRefName(char const* fileRefNameInput) const
-> shared_ptr<string const>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = HspStringView{ fileRefNameInput }.to_os_string();

	OsStringView full_path;
	auto ok = p_->resolver_.find_full_path(file_ref_name.as_ref(), full_path);
	if (!ok) {
		return nullptr;
	}

	return std::make_shared<string const>(full_path.to_hsp_string().data());
}

auto VTNodeScript::fetchScriptAll(char const* fileRefName) const
-> std::unique_ptr<string const>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = HspStringView{ fileRefName }.to_os_string();

	OsStringView content;
	auto ok = p_->resolver_.find_script_content(file_ref_name.as_ref(), content);
	if (!ok) {
		return nullptr;
	}

	return std::make_unique<string>(content.to_hsp_string());
}

auto VTNodeScript::fetchScriptLine(hpiutil::SourcePos const& spos) const
-> unique_ptr<string const>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = HspStringView{ spos.fileRefName() }.to_os_string();

	OsStringView line;
	auto ok = p_->resolver_.find_script_line(file_ref_name.as_ref(), (std::size_t)spos.line(), line);
	if (!ok) {
		return nullptr;
	}

	return std::make_unique<string>(line.to_hsp_string());
}
