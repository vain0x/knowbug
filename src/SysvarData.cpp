// SysvarData を使用する関数

#include <cassert>
#include <cstring>
#include <algorithm>

#include "module/ptr_cast.h"
#include "main.h"
#include "SysvarData.h"

namespace Sysvar
{

//------------------------------------------------
// システム変数名からインデックス値を求める
//------------------------------------------------
Id trySeek( char const* name )
{
	assert(name);
	for ( int i = 0; i < Count; ++ i ) {
		if ( strcmp(name, List[i].name) == 0 ) return static_cast<Id>(i);
	}
	return Id::MAX;
}

//------------------------------------------------
// 整数値のシステム変数へのポインタを得る
//------------------------------------------------
int& getIntRef(Id id)
{
	assert(List[id].type == HSPVAR_FLAG_INT && id != Id::Cnt);
	switch ( id ) {
		case Id::Stat:    return ctx->stat;
		case Id::IParam:  return ctx->iparam;
		case Id::WParam:  return ctx->wparam;
		case Id::LParam:  return ctx->lparam;
		case Id::StrSize: return ctx->strsize;
		case Id::Looplev: return ctx->looplev;
		case Id::Sublev:  return ctx->sublev;
		case Id::Err: {
			static_assert(sizeof(int) == sizeof(ctx->err), "");
			return reinterpret_cast<int&>(ctx->err);
		}
		default: assert_sentinel;
	}
}

//------------------------------------------------
// thismod の値
// 
// (failure: nullptr)
//------------------------------------------------
FlexValue* tryGetThismod()
{
	if ( ctx->prmstack ) {
		auto const thismod = ptr_cast<MPModVarData*>(ctx->prmstack);
		if ( thismod->magic == MODVAR_MAGICCODE ) {
			PVal* const pval = thismod->pval;
			if ( pval->flag == HSPVAR_FLAG_STRUCT ) {
				auto const fv = ptr_cast<FlexValue*>(hpiutil::PVal_getPtr(pval, thismod->aptr));
				return fv;
			}
		}
	}
	return nullptr;
}

//------------------------------------------------
// メモリダンプするための情報を得る
//------------------------------------------------
std::pair<void const*, size_t> tryDump(Id id)
{
	switch ( id ) {
		case Id::Refstr:  return std::make_pair(ctx->refstr, HSPCTX_REFSTR_MAX);
		case Id::Refdval: return std::make_pair(&ctx->refdval, sizeof(double));
		case Id::Cnt:     return std::make_pair(&ctx->mem_loop[1], ctx->looplev * sizeof(LOOPDAT));
		case Id::NoteBuf: {
			PVal* const pval = ctx->note_pval;
			APTR  const aptr = ctx->note_aptr;
			if ( pval && pval->flag == HSPVAR_FLAG_STR ) {
				int size;
				auto const data = hpiutil::varproc(HSPVAR_FLAG_STR)->GetBlockSize(
					pval,
					ptr_cast<PDAT*>(hpiutil::PVal_getPtr(pval, aptr)),
					&size
				);
				return std::make_pair(data, static_cast<size_t>(size));
			}
			break;
		}
		case Sysvar::Id::Thismod:
			if ( auto const fv = Sysvar::tryGetThismod() ) {
				return std::make_pair(fv->ptr, fv->size);
			}
			break;
		default:
			if ( Sysvar::List[id].type == HSPVAR_FLAG_INT ) {
				return std::make_pair(&Sysvar::getIntRef(id), sizeof(int));
			}
			break;
	};
	// no dump
	return std::make_pair(nullptr, 0);
}

} //namespace Sysvar
