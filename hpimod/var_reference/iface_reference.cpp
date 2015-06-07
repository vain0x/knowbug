/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 |
 *		hsp plugin interface (reference)
 |
 *				author uedai
 |
.*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "iface_reference.h"
#include "vt_reference.h"
#include "cmd_reference.h"

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"

// 関数宣言
static int  hpi_cmdfunc( int cmd );
static int hpi_termfunc( int option );

static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

//##########################################################
//        HPI処理
//##########################################################
//------------------------------------------------
// HPI登録関数
//------------------------------------------------
EXPORT void WINAPI hsp3typeinfo_reference( HSP3TYPEINFO* info )
{
	hsp3sdk_init( info );			// SDKの初期化(最初に行なって下さい)
	
	HSPVAR_COREFUNC corefunc = HspVarReference_Init;
	registvar( -1, corefunc );		// 新規型を追加
	
	info->cmdfunc  = hpi_cmdfunc;		// 実行関数(cmdfunc)の登録
	info->reffunc  = hpi_reffunc<&ProcFuncCmd, &ProcSysvarCmd>;		// 参照関数(reffunc)の登録
	info->termfunc = hpi_termfunc;		// 終了関数(termfunc)の登録
	
	return;
}

//------------------------------------------------
// 終了時
//------------------------------------------------
static int hpi_termfunc(int option)
{
	return 0;
}

//##########################################################
//        コマンド処理
//##########################################################
//------------------------------------------------
// 命令
//------------------------------------------------
static int hpi_cmdfunc( int cmd )
{
	code_next();
	
	switch ( cmd ) {
		case 0x000: ReferenceNew();    break;
		case 0x001: ReferenceDelete(); break;
		
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	
	return RUNMODE_RUN;
}

//------------------------------------------------
// 関数
//------------------------------------------------
static int ProcFuncCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x000: ReferenceNew(); break;
		case 0x100: ReferenceMemberOf(); break;
			
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return SetReffuncResult( ppResult, VtReference::IdxDummy );
}

//------------------------------------------------
// システム変数
//------------------------------------------------
static int ProcSysvarCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x000: return SetReffuncResult( ppResult, (int)g_vtReference );	// reference
		
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}
