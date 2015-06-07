// PValRef
#if 0
#include "hsp3plugin_custom.h"
#include "mod_makepval.h"

#include "PValRef.h"

namespace hpimod {

// クラス変数
PVALREF_DBGCODE(
	int PValRef::stt_counter = 1;
)

PValRef* const PValRef::MagicNull = (PValRef*)HspTrue;

//------------------------------------------------
// 構築(new & constructor)
//------------------------------------------------
PValRef* PValRef::New( int vflag )
{
	PValRef* self = (PValRef*)hspmalloc( sizeof(PValRef) );
	if ( vflag > 0 ) PVal_init( &self->pval, vflag );
	self->cntRefed = 0;

	PVALREF_DBGCODE(
		self->id = PValRef::stt_counter ++;		// 参照カウンタ表示用にID値をつける
		dbgout( "[%d] new!", self->id );
	)
	return self;
}

//------------------------------------------------
// 解体(delete & destructor)
//------------------------------------------------
void PValRef::Delete( PValRef* pval )
{
	assert(!!pval);

	PVal_free( AsPVal(pval) );
	hspfree( pval );
	return;
}

//------------------------------------------------
// 参照カウンタの増減
//------------------------------------------------
void PValRef::AddRef( PValRef* pval )
{
	assert(!!pval);

	pval->cntRefed ++;
	PVALREF_DBGCODE(
		dbgout( "[%d] ++ → %d", pval->id, pval->cntRefed );
	)
	return;
}

void PValRef::Release( PValRef* pval )
{
	assert(!!pval);

	pval->cntRefed --;
	PVALREF_DBGCODE(
		dbgout( "[%d] -- → %d", pval->id, pval->cntRefed );
	)
	// カウンタが 0 以下 => 解放
	if ( pval->cntRefed == 0 ) PValRef::Delete( pval );
	return;
}

}	// namespace hpimod
#endif