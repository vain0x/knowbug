// SysvarData を使用する関数

#include <cassert>
#include <cstring>
#include <algorithm>

#include "module/ptr_cast.h"
#include "main.h"
#include "SysvarData.h"

//------------------------------------------------
// システム変数名からインデックス値を求める
// 
// (failure: SysvarId_MAX)
//------------------------------------------------
SysvarId Sysvar_seek( char const* name )
{
	for ( int i = 0; i < SysvarCount; ++ i ) {
		if ( strcmp( name, SysvarData[i].name ) == 0 ) return static_cast<SysvarId>(i);
	}
	return SysvarId_MAX;
}

//------------------------------------------------
// 整数値のシステム変数へのポインタを得る
//
// @ cnt のみ nullptr を返しうるので注意。
//------------------------------------------------
int* Sysvar_getPtrOfInt(int id)
{
	assert(SysvarData[id].type == HSPVAR_FLAG_INT && id != SysvarId_Cnt);
	switch ( id ) {
		case SysvarId_Stat:    return &ctx->stat;
		case SysvarId_IParam:  return &ctx->iparam;
		case SysvarId_WParam:  return &ctx->wparam;
		case SysvarId_LParam:  return &ctx->lparam;
		case SysvarId_StrSize: return &ctx->strsize;
		case SysvarId_Looplev: return &ctx->looplev;
		case SysvarId_Sublev:  return &ctx->sublev;
		case SysvarId_Err:     return ptr_cast<int*>(&ctx->err);
		default: throw;
	}
}

//------------------------------------------------
// thismod の値
// 
// (failure: nullptr)
//------------------------------------------------
FlexValue* Sysvar_getThismod()
{
	if ( ctx->prmstack ) {
		auto const thismod = ptr_cast<MPModVarData*>(ctx->prmstack);
		if ( thismod->magic == MODVAR_MAGICCODE ) {
			PVal* const pval = thismod->pval;
			if ( pval->flag == HSPVAR_FLAG_STRUCT ) {
				auto const fv = ptr_cast<FlexValue*>(hpimod::PVal_getPtr(pval, thismod->aptr));
				return fv;
			}
		}
	}
	return nullptr;
}

//------------------------------------------------
// メモリダンプするための情報を得る
//------------------------------------------------
void Sysvar_getDumpInfo(int id, void const*& out_data, size_t& out_size)
{
	switch ( id ) {
		case SysvarId_Refstr:
			out_data = ctx->refstr; out_size = HSPCTX_REFSTR_MAX;
			return;

		case SysvarId_Refdval:
			out_data = &ctx->refdval; out_size = sizeof(double);
			return;

		case SysvarId_Cnt:
			out_data = &ctx->mem_loop[1]; out_size = ctx->looplev * sizeof(LOOPDAT);
			return;

		case SysvarId_NoteBuf:
		{
			PVal* const pval = ctx->note_pval;
			APTR  const aptr = ctx->note_aptr;
			if ( pval && pval->flag == HSPVAR_FLAG_STR ) {
				int size;
				out_data = hpimod::getHvp(HSPVAR_FLAG_STR)->GetBlockSize(
					pval,
					ptr_cast<PDAT*>(hpimod::PVal_getPtr(pval, aptr)),
					&size
				);
				out_size = static_cast<size_t>(size);
				return;
			} else {
				break;
			}
		}
		case SysvarId_Thismod:
			if ( auto const fv = Sysvar_getThismod() ) {
				out_data = fv->ptr; out_size = fv->size;
				return;
			} else {
				break;
			}

		default:
			// 整数値
			if ( SysvarData[id].type == HSPVAR_FLAG_INT ) {
				out_data = Sysvar_getPtrOfInt(id); out_size = sizeof(int);
				return;
			} else {
				break;
			}
	};

	// no dump
	out_data = nullptr; out_size = 0;
}
