// debug info

#ifndef IG_STRUCT_DEBUG_INFO_H
#define IG_STRUCT_DEBUG_INFO_H

#include <memory>
#include "hsp3debug.h"
#include "hsp3struct.h"
#include "hspvar_core.h"
#include "module/strf.h"
#include "CAx.h"

using hpimod::CAx;

struct DebugInfo
{
public:
	HSP3DEBUG* const debug;
	std::unique_ptr<CAx> const ax;

public:
	DebugInfo(HSP3DEBUG* debug)
		: debug(debug)
		, ax(new CAx())
	{ }

	// 現在実行の実行位置を表す文字列 (更新はしない)
	std::string getCurInfString() const {
		auto const fname = (debug->fname ? debug->fname : "(nameless)");
		return strf("#%d \"%s\"", debug->line, fname);
	}
};

#endif
