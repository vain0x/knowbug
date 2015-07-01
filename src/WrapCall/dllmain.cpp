/******************************************************************************
*
*		WrapCall hpi
*				author uedai @2010 06/01 (Tue)
*
******************************************************************************/

#include "dllmain.h"
#include "type_modcmd.h"
#include "WrapCall.h"

//#include "mod_varutil.h"

// 変数宣言
//static HSP3TYPEINFO* g_pTypeinfo = NULL;
extern int g_pluginType_WrapCall = -1;

// 関数宣言
static int   cmdfunc( int cmd );
static void* reffunc( void* type_res, int cmd );
static int  termfunc( int option );

extern int   ProcFunc( int cmd, void** ppResult );
extern int ProcSysvar( int cmd, void** ppResult );

//------------------------------------------------
// 命令コマンド呼び出し関数
//------------------------------------------------
static int cmdfunc( int cmd )
{
	code_next();
	
	switch( cmd ) {
		// WrapCall_init
		case 0x000:
		{
		//	modcmd_init( &g_pTypeinfo[TYPE_MODCMD] );
			WrapCall::init();
			break;
		}
		
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return RUNMODE_RUN;
}

/*
//------------------------------------------------
// 関数コマンド呼び出し関数
//------------------------------------------------
void* reffunc( int* type_res, int cmd )
{
	void* pResult = NULL;
	
	// '('で始まるかを調べる : 始まらなければシステム変数
	if ( *type != TYPE_MARK || *val != '(' ) {
		
		*type_res = ProcSysvar( cmd, &pResult );
		
	} else {
		
		code_next();
		
		// コマンド分岐
		*type_res = ProcFunc( cmd, &pResult );
		
		// '('で終わるかを調べる
		if ( *type != TYPE_MARK || *val != ')' ) puterror( HSPERR_INVALID_FUNCPARAM );
		code_next();
		
	}
	
	if ( pResult == NULL ) puterror( HSPERR_NORETVAL );
	
	return pResult;
}

//------------------------------------------------
// 関数コマンド処理関数
//------------------------------------------------
int ProcFunc( int cmd, void** ppResult )
{
	switch ( cmd ) {
	//	case 0x000:	return Call          (ppResult);	// call()
		
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}

//------------------------------------------------
// システム変数コマンド処理関数
//------------------------------------------------
int ProcSysvar( int cmd, void** ppResult )
{
	switch ( cmd ) {
	//	case 0x030: return SetReffuncResult( ppResult, HSPVAR_FLAG_CALLER );
		case 0x100:
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}
//*/

//------------------------------------------------
// 終了時呼び出し関数
//------------------------------------------------
int termfunc( int option )
{
//	modcmd_term( &g_pTypeinfo[TYPE_MODCMD] );
	WrapCall::term();
	return 0;
}

//------------------------------------------------
// プラグイン初期化関数
//------------------------------------------------
EXPORT void WINAPI hsp3hpi_init( HSP3TYPEINFO* info )
{
	g_pluginType_WrapCall = info->type;
	HSP3TYPEINFO* infoModcmd = info - (int)g_pluginType_WrapCall + TYPE_MODCMD;
	
	hsp3sdk_init( info );			// SDKの初期化(最初に行なって下さい)
	
	info->cmdfunc  = cmdfunc;		// 実行関数(cmdfunc)の登録
//	info->reffunc  = reffunc;		// 参照関数(reffunc)の登録
	info->termfunc = termfunc;		// 終了関数(termfunc)の登録
	
	// 初期化
	{
		modcmd_init( infoModcmd );
	//	WrapCall::init();
	}
	
	return;
}

//------------------------------------------------
// エントリーポイント関数
//------------------------------------------------
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved )
{
	return TRUE;
}
