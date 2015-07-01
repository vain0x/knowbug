// knowbug - main header

#ifndef IG_HSP3DBGWIN_KNOWBUG_H
#define IG_HSP3DBGWIN_KNOWBUG_H

#include <Windows.h>

#include <string>
using std::string;

#include "chruntime/hsp3debug.h"
#include "chruntime/hsp3struct.h"			// hsp3 core define
#include "hsp3plugin.compatible.h"
#undef stat
#undef min
#undef max

//#include "chruntime/mod_vector.h"

extern HSP3DEBUG* g_debug;
//extern HSPCTX*       ctx;	// hsp3plugin で宣言・定義される
//extern HSPEXINFO* exinfo;

extern HINSTANCE g_hInstance;

struct DebugInfo;
extern DebugInfo* g_dbginfo;

#define dbgmsg(msg) MessageBox( NULL, msg, "Debug Window", MB_OK )

// knowbug コントロール
namespace Knowbug
{

extern void run();
extern void runStop();
extern void runStepIn();
extern void runStepOver();
extern void runStepOut();
extern void runStepOut( int sublev );

extern void logmes( const char* msg );

}

static inline const char* STRUCTDAT_getName(const STRUCTDAT* pStDat) { return &ctx->mem_mds[pStDat->nameidx]; }

#endif
