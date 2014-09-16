/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 |
 *		hsp plugin interface (opex)
 |
 *				author uedai
 |
.*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "iface_opex.h"
#include "cmd_opex.h"

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"

using namespace hpimod;

static int   ProcSttmCmd( int cmd );
static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

//##########################################################
//        HPI処理
//##########################################################
//------------------------------------------------
// HPI登録関数
//------------------------------------------------
EXPORT void WINAPI hsp3typeinfo_opex( HSP3TYPEINFO* info )
{
	hsp3sdk_init( info );

	info->cmdfunc  = cmdfunc<ProcSttmCmd>;
	info->reffunc  = reffunc<ProcFuncCmd, ProcSysvarCmd>;
	//info->termfunc = termfunc;
	return;
}

//##########################################################
//        コマンド処理
//##########################################################
//------------------------------------------------
// 命令
//------------------------------------------------
static int ProcSttmCmd( int cmd )
{
	switch ( cmd ) {
		case 0x000: assign(); break;
		case 0x001:   swap(); break;
		case 0x002:  clone(); break;
		case 0x004: memberOf(); break;
		case 0x005: memberClone(); break;

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
		case 0x000: return assign(ppResult);
		case 0x001: return swap(ppResult);
		case 0x002: return clone(ppResult);
		case 0x003: return castTo(ppResult);
		case 0x004: return memberOf(ppResult);

		case 0x100: return shortLogOp( ppResult, true  );	// and
		case 0x101: return shortLogOp( ppResult, false );	// or
		case 0x102: return   cmpLogOp( ppResult, true  );	// cmp: and
		case 0x103: return   cmpLogOp( ppResult, false );	// cmp: or

		case 0x104: return which( ppResult );
		case 0x105: return what( ppResult );

		case 0x106: return exprs( ppResult );

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}

//------------------------------------------------
// システム変数
//------------------------------------------------
static int ProcSysvarCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case OpexCmd::ConstPtr: return kw_constptr( ppResult );		// keyword

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}
