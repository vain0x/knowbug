/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*
 |
 *		hsp plugin interface (vector)
 |
 *				author uedai (from 2011 07/18(Mon))
 |
.*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#include "iface_vector.h"
#include "vt_vector.h"
#include "cmd_vector.h"
#include "sub_vector.h"

#include "mod_varutil.h"
#include "mod_func_result.h"

#include "knowbug/knowbugForHPI.h"

using namespace hpimod;

static int hpi_termfunc( int option );

static int   ProcSttmCmd( int cmd );
static int   ProcFuncCmd( int cmd, void** ppResult );
static int ProcSysvarCmd( int cmd, void** ppResult );

//------------------------------------------------
// HPIìoò^ä÷êî
//------------------------------------------------
EXPORT void WINAPI hpi_vector( HSP3TYPEINFO* info )
{
	hsp3sdk_init( info );			// SDKÇÃèâä˙âª(ç≈èâÇ…çsÇ»Ç¡Çƒâ∫Ç≥Ç¢)

	info->cmdfunc  = hpimod::cmdfunc<ProcSttmCmd>;
	info->reffunc  = hpimod::reffunc<ProcFuncCmd, ProcSysvarCmd>;
	info->termfunc = hpi_termfunc;

	// êVãKå^Çí«â¡
	registvar(-1, HspVarVector_Init);
	return;
}

//------------------------------------------------
// èIóπéû
//------------------------------------------------
static int hpi_termfunc( int option )
{
	VectorCmdTerminate();

	terminateKnowbugForHPI();
	return 0;
}

//------------------------------------------------
// ñΩóﬂ
//------------------------------------------------
static int ProcSttmCmd( int cmd )
{
	switch ( cmd ) {
		case 0x000: dimtypeEx( g_vtVector ); break;
		case 0x001: VectorDelete(); break;
		case 0x002: VectorChain(false);  break;
		case 0x003: VectorChain(true);   break;

		case 0x010: VectorDimtype();   break;
		case 0x011: VectorClone(); break;
#if 0
		case 0x013: VectorInsert();    break;
		case 0x014: VectorInsert1();   break;
		case 0x015: VectorPushFront(); break;
		case 0x016: VectorPushBack();  break;
		case 0x017: VectorRemove();    break;
		case 0x018: VectorRemove1();   break;
		case 0x019: VectorPopFront();  break;
		case 0x01A: VectorPopBack();   break;
		case 0x01B: VectorReplace();   break;

		case VectorCmdId::Move:
		case VectorCmdId::Swap:
		case VectorCmdId::Rotate:
		case VectorCmdId::Reverse:
			VectorMoving( cmd );
			break;
#endif
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
		case 0x000: return VectorMake( ppResult );
		case 0x003: return VectorDup( ppResult );

		case 0x011:
			VectorClone();
			return SetReffuncResult( ppResult, 0 );
#if 0
		case 0x013: return VectorInsert   ( ppResult );
		case 0x014: return VectorInsert1  ( ppResult );
		case 0x015: return VectorPushFront( ppResult );
		case 0x016: return VectorPushBack ( ppResult );
		case 0x017: return VectorRemove   ( ppResult );
		case 0x018: return VectorRemove1  ( ppResult );
		case 0x019: return VectorPopFront ( ppResult );
		case 0x01A: return VectorPopBack  ( ppResult );
		case 0x01B: return VectorReplace  ( ppResult );

		case VectorCmdId::Move:
		case VectorCmdId::Swap:
		case VectorCmdId::Rotate:
		case VectorCmdId::Reverse:
			return VectorMovingFunc( ppResult, cmd );
#endif

		case 0x100:	return VectorVarinfo( ppResult );
		case 0x101:	return VectorSize( ppResult );
		case 0x102: return VectorSlice( ppResult );
		case 0x103: return VectorSliceOut( ppResult );
		case 0x104: return VectorResult( ppResult );
		case 0x105: return VectorExpr( ppResult );
		case 0x106: return VectorJoin( ppResult );
		case 0x107: return VectorAt( ppResult );
		case 0x200: return VectorIsNull( ppResult );

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
		case 0x000: return SetReffuncResult( ppResult, g_vtVector );

		case 0x200:
		{
			static vector_t const VectorNull { nullptr };
			*ppResult = const_cast<void*>(static_cast<void const*>(&VectorNull));
			return g_vtVector;
		}

		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return 0;
}
