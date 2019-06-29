#include <unordered_set>
#include <tchar.h>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "SourceFileResolver.h"

VTNodeScript::VTNodeScript(std::shared_ptr<SourceFileResolver> resolver)
	: resolver_(resolver)
{
}

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
	auto ok = resolver_->find_full_path(file_ref_name.as_ref(), full_path);
	if (!ok) {
		return nullptr;
	}

	return std::make_shared<string const>(full_path.to_hsp_string().data());
}

auto VTNodeScript::fetchScriptAll(char const* fileRefName) const
-> std::unique_ptr<OsString>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = HspStringView{ fileRefName }.to_os_string();

	auto&& content_opt = resolver_->find_script_content(file_ref_name.as_ref());
	if (!content_opt) {
		return nullptr;
	}

	return std::make_unique<OsString>(content_opt->to_owned());
}

auto VTNodeScript::fetchScriptLine(hpiutil::SourcePos const& spos) const
-> unique_ptr<string const>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = HspStringView{ spos.fileRefName() }.to_os_string();

	auto&& line_opt = resolver_->find_script_line(file_ref_name.as_ref(), (std::size_t)spos.line());
	if (!line_opt) {
		return nullptr;
	}

	return std::make_unique<string>(line_opt->to_hsp_string());
}
