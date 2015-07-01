// WrapCall - type modcmd

// @ TYPE_MODCMD の処理をラップする。

#include "type_modcmd.h"
#include "WrapCall.h"

// 変数定義
static int   (* g_modcmd_cmdfunc_impl)( int )       = NULL;
static void* (* g_modcmd_reffunc_impl)( int*, int ) = NULL;

//------------------------------------------------
// TYPE_MODCMD の処理を乗っ取る
//------------------------------------------------
void modcmd_init( HSP3TYPEINFO* info )
{
	g_modcmd_cmdfunc_impl = info->cmdfunc;
	g_modcmd_reffunc_impl = info->reffunc;
	
	info->cmdfunc = modcmd_cmdfunc;
	info->reffunc = modcmd_reffunc;
	return;
}

//------------------------------------------------
// TYPE_MODCMD の処理を元に戻す
//------------------------------------------------
void modcmd_term( HSP3TYPEINFO* info )
{
	info->cmdfunc = g_modcmd_cmdfunc_impl;
	info->reffunc = g_modcmd_reffunc_impl;
	return;
}

//------------------------------------------------
// [modcmd] 命令コマンド
//------------------------------------------------
int modcmd_cmdfunc( int cmdid )
{
	STRUCTDAT* const pStDat = &ctx->mem_finfo[cmdid];
	
	WrapCall::bgnCall( pStDat );
	
	int result = g_modcmd_cmdfunc_impl( cmdid );	// 常に RUNMODE_RUN を返却する (HSP3.3現在)
	
	int resultWrap = WrapCall::endCall();
	
	if ( result == RUNMODE_RUN && resultWrap != result ) {
		result = resultWrap;
	}
	
	return result;
}

//------------------------------------------------
// [modcmd] 関数コマンド
//------------------------------------------------
void* modcmd_reffunc( int* type_res, int cmdid )
{
	WrapCall::bgnCall( &ctx->mem_finfo[cmdid] );
	
	void* result = g_modcmd_reffunc_impl( type_res, cmdid );
	
	WrapCall::endCall( result, *type_res );
	return result;
}
