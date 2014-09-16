/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 |
 *		hsp plugin interface (assoc)
 |
 *				author uedai
 |
.*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "iface_assoc.h"
#include "CAssoc.h"
#include "vt_assoc.h"
#include "cmd_assoc.h"

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"

#include "knowbug/knowbugForHPI.h"

static int   termfunc( int option );

static int   ProcSttmCmd( int cmd );
static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

//------------------------------------------------
// HPIìoò^ä÷êî
//------------------------------------------------
EXPORT void WINAPI hpi_assoc( HSP3TYPEINFO* info )
{
	hsp3sdk_init( info );			// SDKÇÃèâä˙âª(ç≈èâÇ…çsÇ»Ç¡Çƒâ∫Ç≥Ç¢)

	info->cmdfunc  = hpimod::cmdfunc<ProcSttmCmd>;
	info->reffunc  = hpimod::reffunc<ProcFuncCmd, ProcSysvarCmd>;
	info->termfunc = termfunc;

	// êVãKå^Çí«â¡
	registvar(-1, HspVarAssoc_Init);
	return;
}

//------------------------------------------------
// èIóπéû
//------------------------------------------------
static int termfunc(int option)
{
	AssocTerm();
	terminateKnowbugForHPI();
	return 0;
}

//------------------------------------------------
// ñΩóﬂ
//------------------------------------------------
static int ProcSttmCmd( int cmd )
{
	switch ( cmd ) {
		case 0x000: AssocNew();    break;
		case 0x001: AssocDelete(); break;
		case 0x002: AssocClear();  break;
		case 0x003: AssocChain();  break;
		case 0x004: AssocCopy();   break;

		case 0x010: AssocImport(); break;
		case 0x011: AssocInsert(); break;
		case 0x012: AssocRemove(); break;

		case 0x020: AssocDim();   break;
		case 0x021: AssocClone(); break;

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
		case 0x000: return AssocNewTemp(ppResult);
		case 0x004: return AssocNewTempDup(ppResult);

		case 0x021:
			AssocClone();
			return SetReffuncResult( ppResult, 0 );		// ìYéö 0 Çï‘Ç∑

		case 0x100:	return AssocVarinfo(ppResult);
		case 0x101: return AssocSize(ppResult);
		case 0x102: return AssocExists(ppResult);
		case 0x103: return AssocForeachNext(ppResult);
		case 0x104: return AssocResult( ppResult );
		case 0x105: return AssocExpr( ppResult );

		case 0x200: return AssocIsNull( ppResult );

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}

//------------------------------------------------
// ÉVÉXÉeÉÄïœêî
//------------------------------------------------
static int ProcSysvarCmd( int cmd, void** ppResult )
{
	switch ( cmd ) {
		case 0x000: return hpimod::SetReffuncResult( ppResult, g_vtAssoc );	// assoc

		case 0x200: return SetReffuncResult(ppResult, nullptr);		// AssocNull

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}
