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

// ä÷êîêÈåæ
static int  hpi_cmdfunc( int cmd );
static int hpi_termfunc( int option );

static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

//##########################################################
//        HPIèàóù
//##########################################################
//------------------------------------------------
// HPIìoò^ä÷êî
//------------------------------------------------
EXPORT void WINAPI hsp3typeinfo_reference( HSP3TYPEINFO* info )
{
	hsp3sdk_init( info );			// SDKÇÃèâä˙âª(ç≈èâÇ…çsÇ»Ç¡Çƒâ∫Ç≥Ç¢)
	
	HSPVAR_COREFUNC corefunc = HspVarReference_Init;
	registvar( -1, corefunc );		// êVãKå^Çí«â¡
	
	info->cmdfunc  = hpi_cmdfunc;		// é¿çsä÷êî(cmdfunc)ÇÃìoò^
	info->reffunc  = hpi_reffunc<&ProcFuncCmd, &ProcSysvarCmd>;		// éQè∆ä÷êî(reffunc)ÇÃìoò^
	info->termfunc = hpi_termfunc;		// èIóπä÷êî(termfunc)ÇÃìoò^
	
	return;
}

//------------------------------------------------
// èIóπéû
//------------------------------------------------
static int hpi_termfunc(int option)
{
	return 0;
}

//##########################################################
//        ÉRÉ}ÉìÉhèàóù
//##########################################################
//------------------------------------------------
// ñΩóﬂ
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
// ä÷êî
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
// ÉVÉXÉeÉÄïœêî
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
