
#pragma once

#include <string>
#include "hpiutil/SourcePos.hpp"

#undef max

// FIXME: たいして役に立っていないので削除したい。HspObjects か HspDebugApi に統合したい
// HSP3DEBUG wrapper
class DebugInfo
{
	using string = std::string;

	HSP3DEBUG* const debug_;

public:
	DebugInfo(HSP3DEBUG* debug);
	~DebugInfo();

	bool setStepMode(int mode) { return (debug_->dbg_set(mode) >= 0); }

	auto fetchGeneralInfo() const -> std::vector<std::pair<string, string>>;

	auto curPos() const -> hpiutil::SourcePos const
	{
		return { debug_->fname, debug_->line - 1 };
	}

	auto getCurInfString() const -> string
	{
		return curPos().toString();
	}

	void updateCurInf()
	{
		debug_->dbg_curinf();
	}

	auto file_ref_name() const -> std::string {
		return debug_->fname;
	}

	auto line_index() const -> std::size_t {
		return std::max(0, debug_->line - 1);
	}
};

extern std::unique_ptr<DebugInfo> g_dbginfo;
