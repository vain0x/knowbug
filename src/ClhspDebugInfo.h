// clhsp dbg info

#ifndef IG_CLHSP_DEBUG_INFO_H
#define IG_CLHSP_DEBUG_INFO_H

#include "chruntime/hsp3debug.h"
#include "chruntime/hsp3struct.h"
#include "chruntime/hspvar_core.h"
#include "hsp3compatible.h"

struct DebugInfo
{
	HSPCTX*    ctx;
	HSPEXINFO* exinfo;
	HSP3DEBUG* debug;
};

// その他
static inline const char *getModeString( varmode_t mode )
{
	return	( mode <= HSPVAR_MODE_NONE   ) ? "無効" :
			( mode == HSPVAR_MODE_MALLOC ) ? "実体" :
			( mode == HSPVAR_MODE_CLONE  ) ? "クローン" : "???"
	;
}

#endif
