/******************************************************************************
*
*		Call hpi
*				author uedai @2008 11/2(Sun) -> 2012 09/18 (Tue)
*
******************************************************************************/

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"
#include "mod_varutil.h"

#include "iface_call.h"
#include "cmd_call.h"
#include "cmd_method.h"
#include "cmd_modcls.h"
#include "vt_functor.h"

using namespace hpimod;

// •Ï”éŒ¾
int g_pluginType_call = -1;

// ŠÖ”éŒ¾
static int termfunc( int option );

static int  ProcSttmCmd( int cmd );
static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

static void wrap_reffunc_intfunc( HSP3TYPEINFO* info );

//##########################################################
//        HPIˆ—
//##########################################################
//------------------------------------------------
// HPI“o˜^ŠÖ”
//------------------------------------------------
EXPORT void WINAPI hsp3typeinfo_call(HSP3TYPEINFO* info)
{
	g_pluginType_call = info->type;

	hsp3sdk_init( info );			// SDK‚Ì‰Šú‰»(Å‰‚És‚È‚Á‚Ä‰º‚³‚¢)

	info->cmdfunc  = cmdfunc<&ProcSttmCmd>;	
	info->reffunc  = reffunc<&ProcFuncCmd, &ProcSysvarCmd>;
	info->termfunc = termfunc;

	// functor Œ^‚ð“o˜^
	registvar( -1, reinterpret_cast<HSPVAR_COREFUNC>( HspVarFunctor_init ) );

	// TYPE_INTFUNC ‚Ìƒ‰ƒbƒsƒ“ƒO
//	wrap_reffunc_intfunc( &(info - g_pluginType_call)[TYPE_INTFUNC] );	// info ‚ª HSP3TYPEINFO[] ‚Å‚ ‚é‚±‚Æ‚ª‘O’ñ

	return;
}

//------------------------------------------------
// –½—ß
//------------------------------------------------
static int ProcSttmCmd( int cmd )
{
	switch ( cmd ) {
		case 0x000: Call();             break;	// call
		case 0x001: Call_alias();       break;	// call_alias
		case 0x002: Call_aliasAll();    break;	// call_aliasAll
		case CallCmdId::RetVal:
					Call_retval();      break;	// call_retval
		case 0x004: Call_declare();     break;	// call_dec

		case 0x010: Call_StreamBegin(); break;	// call_stream_begin
		case 0x011: Call_StreamLabel(); break;	// call_stream_label
		case 0x012: Call_StreamAdd();   break;	// call_stream_add
		case 0x013: Call_StreamEnd();   break;	// call_stream_end
		case 0x014: Call_StreamCallerAdd(); break;	// 

		case 0x020: Method_replace();   break;	// method_replace
		case 0x021: Method_add();       break;	// method_add
//		case 0x022: Method_cloneThis(); break;	// method_cloneThis

		case 0x030: dimtypeEx( HSPVAR_FLAG_FUNCTOR ); break;	// functor

		case 0x040: CallCmd_sttm();     break;	// callcs

		case 0x050: ModCls_Init();       break;	// modcls_init
		case 0x051: ModCls_Term();       break;	// modcls_term
		case 0x052: ModCls_Register();   break;	// modcls_register
		case 0x053: ModCls_Newmod();     break;	// newmod
		case 0x054: ModCls_Delmod();     break;	// delmod
		case 0x055: ModCls_Dupmod();     break;	// dupmod

	//	case 0x060: Call_CoBegin();     break;	// co_begin
	//	case 0x061: Call_CoEnd();       break;	// co_end
	//	case 0x062: Call_CoYield();     break;	// co_yield
		case 0x063: Call_CoYieldImpl(); break;	// co_yield_impl
	//	case 0x064: Call_CoExit();      break;	// co_exit

		case CallCmdId::LambdaBody: Call_LambdaBody(); break;	// lambda ‚ª“à•”‚ÅŒÄ‚Ô–½—ß
#ifdef _DEBUG
		case 0x0FF: CallHpi_test(); break;
#endif
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}

	return RUNMODE_RUN;
}

//------------------------------------------------
// ŠÖ”
//------------------------------------------------
static int ProcFuncCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x000:	return Call          (ppResult);	// call()
		case 0x013: return Call_StreamEnd(ppResult);	// call_stream()
		case 0x030: return Functor_cnv   (ppResult);	// functor()
		case 0x040: return CallCmd_func  (ppResult);	// callcf()
		case 0x053: return ModCls_Newmod (ppResult);	// newmod()
		case 0x055: return ModCls_Dupmod (ppResult);	// dupmod()

		case 0x100: return Call_arginfo  (ppResult);	// call_arginfo()
		case CallCmdId::ArgV:
					return Call_argv     (ppResult);	// call_argv()
		case CallCmdId::ValOf:
		case 0x102: return Call_getLocal (ppResult);	// call_getlocal()
		case 0x103: return Call_result   (ppResult);	// call_result()
		case 0x110: return AxCmdOf       (ppResult);	// axcmdOf()
		case 0x111: return LabelOf       (ppResult);	// labelOf()

		case 0x120: return ArgBind       (ppResult);	// argbind()
		case 0x121: return UnBind        (ppResult);	//  unbind()
		case 0x126: return Call_NewStreamCaller(ppResult);	// stream_call_new
		case CallCmdId::Lambda:
					return Call_Lambda   (ppResult);	// lambda()

		case 0x130: return Functor_argc  (ppResult);	// functor_argc
		case 0x131: return Functor_isFlex(ppResult);	// functor_isFlex

		case 0x150: return ModCls_Identity (ppResult);	// modcls_identity
		case 0x151: return ModCls_Name     (ppResult);	// modcls_name
		case 0x15A: return ModInst_Cls     (ppResult);	// modinst_cls
		case 0x15B: return ModInst_ClsName (ppResult);	// modinst_clsname
		case 0x15C: return ModInst_Identify(ppResult);	// modinst_identify

		case 0x140: return Call_CoCreate(ppResult);	// co_create

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}

	return 0;
}

//------------------------------------------------
// ƒVƒXƒeƒ€•Ï”
//------------------------------------------------
static int ProcSysvarCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x030: return SetReffuncResult( ppResult, (int)HSPVAR_FLAG_FUNCTOR );

		case 0x053: return ModCls_Newmod (ppResult, true);	// newmod()
		case 0x054: return ModCls_Nullmod(ppResult);		// nullmod (delmod)

		case 0x200:	return Call_thislb( ppResult );		// call_thislb
		case 0x250: return ModCls_This( ppResult );		// modcls_thismod

		case CallCmdId::ByRef: //
	//	case CallCmdId::ByThismod:
		case CallCmdId::ByDef:
		case CallCmdId::NoBind:
		case CallCmdId::PrmOf:
		case CallCmdId::ValOf:
		case CallCmdId::NoCall:
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}

//------------------------------------------------
// I—¹Žž
//------------------------------------------------
static int termfunc(int option)
{
	ReleaseBounds();
	Call_Term();
//	Proto_Term();
//	ModOp_Term();
	return 0;
}

//------------------------------------------------
// str, double, int ‚ðƒVƒXƒeƒ€•Ï”‚Æ‚µ‚ÄŽg‚¦‚é‚æ‚¤‚É‚·‚é
// 
// @ ‚»‚ê‚¼‚ê‘Î‰ž‚·‚éŒ^ƒ^ƒCƒv’l‚ð•Ô‹p‚·‚éB
//------------------------------------------------
static void*(*reffunc_intfunc_impl)(int*, int);

int reffunc_intfunc_procSysvar( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x000: return SetReffuncResult( ppResult, HSPVAR_FLAG_INT );		// int()
		case 0x100: return SetReffuncResult( ppResult, HSPVAR_FLAG_STR );		// str()
		case 0x185: return SetReffuncResult( ppResult, HSPVAR_FLAG_DOUBLE );	// double()
		default:    puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;	// Œx—}§
}

void* reffunc_intfunc_wrap( int* type_res, int cmd )
{
	if ( !(*type == TYPE_MARK && *val == '(') ) {
		void* pResult;
		*type_res = reffunc_intfunc_procSysvar( cmd, &pResult );
		return pResult;
	} else {
		return reffunc_intfunc_impl( type_res, cmd );
	}
}

void wrap_reffunc_intfunc(HSP3TYPEINFO* info)
{
	if ( info->type != TYPE_INTFUNC ) return;

	reffunc_intfunc_impl = info->reffunc;
	info->reffunc = reffunc_intfunc_wrap;
	return;
}
