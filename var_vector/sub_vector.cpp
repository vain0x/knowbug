// vector - SubCommand code

#include "iface_vector.h"
#include "sub_vector.h"
#include "vt_vector.h"

#include "mod_makepval.h"

using namespace hpimod;

bool isValidIndex(vector_t const& self, int idx)
{
	return (!self.isNull() && 0 <= idx && static_cast<size_t>(idx) < self->size());
}

bool isValidRange(vector_t const& self, size_t iBgn, size_t iEnd)
{
	return !self.isNull() && iBgn < iEnd && iEnd <= self->size();
}

void chainShallow(vector_t& dst, vector_t const& src, std::pair<size_t, size_t> range)
{
	assert(!dst.isNull() && isValidRange(src, range.first, range.second));

	if ( !src.isNull() ) {
		dst->insert(dst->end(), src->begin() + range.first, src->begin() + range.second);
	}
	return;
}

void chainDeep(vector_t& dst, vector_t const& src, std::pair<size_t, size_t> range)
{
	assert(!dst.isNull() && isValidRange(src, range.first, range.second));

	if ( !src.isNull() ) {
		size_t const offset = dst->size();
		size_t const lenRange = (range.second - range.first);
		dst->resize(offset + lenRange);
		for ( size_t i = 0; i < lenRange; ++i ) {
			PVal_copy(dst->at(i).valuePtr(), src->at(range.first + i).valuePtr());
		}
	}
	return;
}

#if 0
//------------------------------------------------
// 内部変数の取得
// 
// @ 本体が参照されているときは null を返す。
//------------------------------------------------
PVal* Vector_getPValPtr( PVal* pval )
{
	if ( pval->arraycnt == 0 ) return nullptr;
	return Vector_getPValPtr( pval, pval->offset );
}

//------------------------------------------------
// 内部変数の取得
//------------------------------------------------
PVal* Vector_getPValPtr( PVal* pval, int idx )
{
	return Vector_getPtr(pval)->At( idx );
}

//------------------------------------------------
// 内部変数の実体ポインタを取得
//------------------------------------------------
void* Vector_getRealPtr( PVal* pval )
{
	PVal* pvdat = Vector_getPValPtr( pval );
	if ( !pvdat ) return nullptr;

	return getHvp( pvdat->flag )->GetPtr( pvdat );
}

//------------------------------------------------
// 実体への参照
//------------------------------------------------
CVector*& Vector_getPtr(PVal* pval)
{
	return VectorTraits::derefValptr(pval->pt);
}

//------------------------------------------------
// 実体ポインタを取得
//------------------------------------------------
CVector** Vector_getValptr( PVal* pval )
{
	return (CVector**)(&pval->master);
}
#endif

//------------------------------------------------
// 複写
//------------------------------------------------
void Vector_copy( PVal* pval, vector_t src )
{
	// 型が異なる => 左辺を初期化する (元々のデータは消滅)
	if ( pval->flag != g_vtVector ) {
		exinfo->HspFunc_dim( pval, g_vtVector, 0, 1, 0, 0, 0 );
	}

	auto& dst = VectorTraits::getMaster( pval );
	dst = vector_t::make();
	dst.incRef();

	if ( !src.isNull() ) {
		dst->insert(dst->end(), src->begin(), src->end());
	}
	return;
}
