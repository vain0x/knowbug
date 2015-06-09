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
	HSP3DEBUG* const debug;
	std::unique_ptr<hpimod::CAx> const ax;

public:
	DebugInfo(HSP3DEBUG* debug);
	~DebugInfo();

	// 現在実行の実行位置を表す文字列 (更新はしない)
	string getCurInfString() const;
	static string formatCurInfString(char const* fname, int line);

	//全般情報
	std::vector<std::pair<string, string>> fetchGeneralInfo() const;

	//静的変数リスト
	std::vector<string> fetchStaticVarNames() const;
};

#endif
