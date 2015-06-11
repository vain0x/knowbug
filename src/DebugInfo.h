// debug info

#ifndef IG_STRUCT_DEBUG_INFO_H
#define IG_STRUCT_DEBUG_INFO_H

#include <memory>
#include <vector>
#include "hsp3debug.h"
#include "hsp3struct.h"
#include "hspvar_core.h"
#include "module/strf.h"

namespace hpimod { class CAx; }

struct DebugInfo
{
	using string = std::string;
public:
	std::unique_ptr<hpimod::CAx> const ax;
private:
	HSP3DEBUG* const debug;

public:
	DebugInfo(HSP3DEBUG* debug);
	~DebugInfo();

	char const* curFileName() const { return debug->fname; }
	int curLine() const { return debug->line - 1; }
	std::vector<std::pair<string, string>> fetchGeneralInfo() const;
	std::vector<string> fetchStaticVarNames() const;

	void updateCurInf() { (debug->dbg_curinf()); }
	bool setStepMode(int mode) { return (debug->dbg_set(mode) >= 0); }

	// 現在実行の実行位置を表す文字列 (更新はしない)
	string getCurInfString() const;
	static string formatCurInfString(char const* fname, int line);
};

#endif
