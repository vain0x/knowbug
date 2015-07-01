// clhsp dbg info

#ifndef IG_CLHSP_DEBUG_INFO_H
#define IG_CLHSP_DEBUG_INFO_H

#include "chruntime/hsp3debug.h"
#include "chruntime/hsp3struct.h"
#include "chruntime/hspvar_core.h"
#include "hsp3compatible.h"

#include "CAx.h"

struct DebugInfo
{
public:
	HSPCTX*    const ctx;
	HSPEXINFO* const exinfo;
	HSP3DEBUG* const debug;

	CAx* const ax;

public:
	DebugInfo(HSPCTX* ctx, HSPEXINFO* exinfo, HSP3DEBUG* debug)
		: ctx(ctx)
		, exinfo(exinfo)
		, debug(debug)
		, ax(new CAx())
	{ }

	~DebugInfo() { delete ax; }
};

// その他
static inline const char* getModeString( varmode_t mode )
{
	return	( mode <= HSPVAR_MODE_NONE   ) ? "無効" :
			( mode == HSPVAR_MODE_MALLOC ) ? "実体" :
			( mode == HSPVAR_MODE_CLONE  ) ? "クローン" : "???"
	;
}

#endif
