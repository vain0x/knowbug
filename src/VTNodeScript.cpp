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
	auto file_ref_name = to_os(as_hsp(fileRefNameInput));

	auto&& full_path_opt = resolver_->find_full_path(as_view(file_ref_name));
	if (!full_path_opt) {
		return nullptr;
	}

	return std::make_shared<string const>(to_hsp(*full_path_opt));
}

auto VTNodeScript::fetchScriptAll(char const* fileRefName) const
-> std::unique_ptr<OsString>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = to_os(as_hsp(fileRefName));

	auto&& content_opt = resolver_->find_script_content(as_view(file_ref_name));
	if (!content_opt) {
		return nullptr;
	}

	return std::make_unique<OsString>(to_owned(*content_opt));
}

auto VTNodeScript::fetchScriptLine(hpiutil::SourcePos const& spos) const
-> unique_ptr<string const>
{
	// FIXME: 無駄なコピーの塊
	auto file_ref_name = to_os(as_hsp(spos.fileRefName()));

	auto&& line_opt = resolver_->find_script_line(as_view(file_ref_name), (std::size_t)spos.line());
	if (!line_opt) {
		return nullptr;
	}

	return std::make_unique<string>(to_hsp(*line_opt));
}
