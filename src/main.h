//ごった煮:
//windows APIまわりの宣言
//hspsdkまわりの宣言
//knowbugの宣言

#ifndef IG_HSP3DBGWIN_KNOWBUG_H
#define IG_HSP3DBGWIN_KNOWBUG_H

#include <Windows.h>
#undef min
#undef max

#include "hsp3plugin.h"
#undef stat
#include "hpimod/basis.h"

#include "DebugInfo.h"
#include "module/utility.h"

using hpimod::vartype_t;
using hpimod::varmode_t;
using hpimod::label_t;
using hpimod::csptr_t;
using hpimod::stdat_t;
using hpimod::stprm_t;
using hpimod::HSPVAR_FLAG_COMOBJ;
using hpimod::HSPVAR_FLAG_VARIANT;

#ifdef with_WrapCall
namespace WrapCall
{
	struct ModcmdCallInfo;
} //namespace WrapCall
struct ResultNodeData;
#endif

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
