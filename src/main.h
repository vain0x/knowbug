//ごった煮:
//windows APIまわりの宣言
//hspsdkまわりの宣言
//knowbugの宣言

#ifndef IG_HSP3DBGWIN_KNOWBUG_H
#define IG_HSP3DBGWIN_KNOWBUG_H

#include <Windows.h>
#undef min
#undef max

#include "hpiutil/hpiutil.hpp"
#include "DebugInfo.h"
#include "module/utility.h"

using hpiutil::vartype_t;
using hpiutil::varmode_t;
using hpiutil::label_t;
using hpiutil::csptr_t;
using hpiutil::stdat_t;
using hpiutil::stprm_t;
using hpiutil::HSPVAR_FLAG_COMOBJ;
using hpiutil::HSPVAR_FLAG_VARIANT;

//extern HSPCTX* ctx; // declared and defined in hsp3plugin.(h/cpp)
//extern HSPEXINFO* exinfo;

// knowbug コントロール
namespace Knowbug
{
	extern HINSTANCE getInstance();

	extern void run();
	extern void runStop();
	extern void runStepIn();
	extern void runStepOver();
	extern void runStepOut();
	extern void runStepReturn(int sublev);

	extern bool isStepRunning();
	extern bool continueConditionalRun();

	extern void logmes(char const* msg); //自動改行なし
	extern void logmesWarning(char const* msg);
} // namespace Knowbug

#endif
