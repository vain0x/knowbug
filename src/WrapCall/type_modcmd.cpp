// WrapCall - type modcmd

// TYPE_MODCMD の処理をラップし、ユーザ定義関数の呼び出し・終了を通知する。

#include "type_modcmd.h"
#include "WrapCall.h"

static int   modcmd_cmdfunc(int cmd);
static void* modcmd_reffunc(int* type_res, int cmd);

// 変数
static int   (*g_modcmd_cmdfunc_impl)(int)       = nullptr;
static void* (*g_modcmd_reffunc_impl)(int*, int) = nullptr;

//------------------------------------------------
// TYPE_MODCMD の処理を乗っ取る
//------------------------------------------------
void modcmd_init(HSP3TYPEINFO* info)
{
	g_modcmd_cmdfunc_impl = info->cmdfunc;
	g_modcmd_reffunc_impl = info->reffunc;

	info->cmdfunc = modcmd_cmdfunc;
	info->reffunc = modcmd_reffunc;
}

//------------------------------------------------
// TYPE_MODCMD の処理を元に戻す
//------------------------------------------------
void modcmd_term(HSP3TYPEINFO* info)
{
	if ( g_modcmd_cmdfunc_impl ) {
		info->cmdfunc = g_modcmd_cmdfunc_impl;
		info->reffunc = g_modcmd_reffunc_impl;

		g_modcmd_cmdfunc_impl = nullptr;
		g_modcmd_reffunc_impl = nullptr;
	}
}

//------------------------------------------------
// [modcmd] 命令コマンド
//------------------------------------------------
int modcmd_cmdfunc(int cmdid)
{
	stdat_t const stdat = hpimod::getSTRUCTDAT(cmdid);

	WrapCall::bgnCall(stdat);
	int const result = g_modcmd_cmdfunc_impl(cmdid);
	WrapCall::endCall();
	return result;
}

//------------------------------------------------
// [modcmd] 関数コマンド
//------------------------------------------------
void* modcmd_reffunc(int* type_res, int cmdid)
{
	stdat_t const stdat = hpimod::getSTRUCTDAT(cmdid);

	WrapCall::bgnCall(stdat);
	PDAT* const result = static_cast<PDAT*>(g_modcmd_reffunc_impl(type_res, cmdid));
	WrapCall::endCall(result, *type_res);
	return result;
}
