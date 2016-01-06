#ifndef IG_STRUCT_DEBUG_INFO_H
#define IG_STRUCT_DEBUG_INFO_H

#include <memory>
#include <vector>
#include "hsp3debug.h"
#include "hsp3struct.h"
#include "hspvar_core.h"

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
	auto fetchStaticVarNames() const -> std::vector<string>;

	// current position data
	auto curFileName() const -> char const*
	{
		return (debug_->fname ? debug_->fname : "???");
	}

	auto  curLine() const -> int
	{
		return debug_->line - 1;
	}

	auto getCurInfString() const -> string
	{
		return formatCurInfString(curFileName(), curLine());
	}

	static auto formatCurInfString(char const* fname, int line) -> string;

	void updateCurInf()
	{
		debug_->dbg_curinf();
	}
};

extern std::unique_ptr<DebugInfo> g_dbginfo;

#endif
