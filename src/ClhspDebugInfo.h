// clhsp dbg info

#ifndef IG_CLHSP_DEBUG_INFO_H
#define IG_CLHSP_DEBUG_INFO_H

#include "hsp3/hsp3debug.h"
#include "hsp3/hsp3struct.h"
#include "hsp3/hspvar_core.h"
#include "hsp3compatible.h"

struct DebugInfo
{
	HSPCTX*    ctx;
	HSPEXINFO* exinfo;
	HSP3DEBUG* debug;
};

#endif
