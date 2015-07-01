// knowbug - main header

// todo: よくわからないヘッダ

#ifndef IG_HSP3DBGWIN_KNOWBUG_H
#define IG_HSP3DBGWIN_KNOWBUG_H

#include <Windows.h>
#undef min
#undef max

#include <string>
using std::string;

#include "hsp3plugin.h"
#undef stat
#include "hpimod/basis.h"

using hpimod::vartype_t;
using hpimod::varmode_t;
using hpimod::label_t;
using hpimod::csptr_t;
using hpimod::stdat_t;
using hpimod::stprm_t;
using hpimod::HSPVAR_FLAG_COMOBJ;
using hpimod::HSPVAR_FLAG_VARIANT;

namespace WrapCall
{
	struct ModcmdCallInfo;
}

//extern HSP3DEBUG* g_debug;
//extern HSPCTX*       ctx;	// hsp3plugin で宣言・定義される
//extern HSPEXINFO* exinfo;

struct DebugInfo;
extern DebugInfo* g_dbginfo;

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

	extern void logmes(char const* msg);
	extern void logmesWarning(char const* msg);

	// WrapCall 側から呼ばれる関数
	extern void bgnCalling(::WrapCall::ModcmdCallInfo const& callinfo);
	extern void endCalling(::WrapCall::ModcmdCallInfo const& callinfo, void* ptr, vartype_t vtype);
} // namespace Knowbug

#endif
