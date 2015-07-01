// 変数データの一行文字列

#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

#include "CVarTree.h"
#include "CVarinfoLine.h"
#include "CVarinfoTree.h"

#include "module/ptr_cast.h"

#ifdef with_Assoc
# include "hpimod/var_assoc/for_knowbug.var_assoc.h"
#endif
#ifdef with_Vector
# include "hpimod/var_vector/for_knowbug.var_vector.h"
#endif
#ifdef with_Array
# include "hpimod/var_array/src/for_knowbug.var_array.h"
#endif

#include "with_ModPtr.h"
#include "WrapCall/ModcmdCallInfo.h"
#include "WrapCall/ResultNodeData.h"

extern string removeScopeResolution(string const& name);	// at CVarinfoTree

//------------------------------------------------
// 構築
//------------------------------------------------
CVarinfoLine::CVarinfoLine( int lenLimit )
	: mBuf(lenLimit)
{
	mBuf.reserve(0x100);
	return;
}

//##########################################################
//        ルート処理関数
//##########################################################
#if 0
//------------------------------------------------
// [add] 単体変数
//------------------------------------------------
void CVarinfoLine::addVarScalar( PVal const* pval )
{
	addVarScalar( pval );
	return;
}
#endif

//------------------------------------------------
// [add] システム変数
//------------------------------------------------
void CVarinfoLine::addSysvar( SysvarId id )
{
	switch ( id ) {
		case SysvarId_Refstr: addValue( HSPVAR_FLAG_STR, ctx->refstr ); break;
		case SysvarId_Refdval: addValue( HSPVAR_FLAG_DOUBLE, &ctx->refdval ); break;

		case SysvarId_Cnt:
			if ( int const lvLoop = ctx->looplev ) {
				addValue( HSPVAR_FLAG_INT, &ctx->mem_loop[lvLoop].cnt );
			} else {
				cat( "(out of loop)" );
			}
			break;

		case SysvarId_NoteBuf:
			if ( PVal* const pval = ctx->note_pval ) {
				addVarScalar(pval, ctx->note_aptr);
			} else {
				cat( "(null)" );
			}
			break;

		case SysvarId_Thismod:
			if ( auto const fv = Sysvar_getThismod() ) {
				addItem_flexValue( fv );
			} else {
				cat("(null)");
			}
			break;
		default:
			// 整数値
			if ( SysvarData[id].type == HSPVAR_FLAG_INT ) {
				addValue(HSPVAR_FLAG_INT, Sysvar_getPtrOfInt(id));
			} else throw;
	};
}

//------------------------------------------------
// [add] 呼び出し
//------------------------------------------------
void CVarinfoLine::addCall(stdat_t stdat, void const* prmstk)
{
	if ( prmstk ) {
		cat(strf("%s(", hpimod::STRUCTDAT_getName(stdat)));
		addPrmstack(stdat, prmstk);
		cat(")");
	} else {
		cat(strf("%s([展開中]) ", hpimod::STRUCTDAT_getName(stdat)));
	}
	return;
}

//------------------------------------------------
// [add] 返値
//------------------------------------------------
void CVarinfoLine::addResult( void const* ptr, vartype_t type )
{
	addValue( type, ptr );
	return;
}

//##########################################################
//        要素ごとの処理関数
//##########################################################
//------------------------------------------------
// [add][item] 値
//------------------------------------------------
void CVarinfoLine::addValue( vartype_t type, void const* ptr )
{
#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
	auto const vtname = hpimod::getHvp(type)->vartype_name;
#endif

	if ( type == HSPVAR_FLAG_STRUCT ) {
		addItem_flexValue( cptr_cast<FlexValue*>(ptr) );
#ifdef with_Assoc
	// assoc 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, assoc_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CAssoc**>(ptr);
		addItem_assoc( src );
#endif
#ifdef with_Vector
	// vector 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, vector_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CVector**>(ptr);
		addItem_vector( src );
#endif
#ifdef with_Array
	// array 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, array_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CArray**>( ptr );
		addItem_array( src );
#endif
#ifdef with_ModPtr
	} else if ( type == HSPVAR_FLAG_INT && ModPtr::isValid(*cptr_cast<int*>(ptr)) ) {
		int const modptr = *cptr_cast<int*>(ptr);
		cat(strf("mp#%d->", ModPtr::getIdx(modptr)));
		addItem_flexValue( ModPtr::getValue(modptr) );
#endif
//	} else if ( type == HSPVAR_FLAG_STR ) {
//		addItem_string( cptr_cast<char*>(ptr) );
		
	// その他
	} else {
		cat( stringizeSimpleValue(type, ptr, true) );
	}
	return;
}

//------------------------------------------------
// [add][item] 変数
//
// @ vector や array は常に単体変数とみなす。
//------------------------------------------------
void CVarinfoLine::addVar( PVal const* pval )
{
#if defined(with_Vector) || defined(with_Array)
	auto const hvp = hpimod::getHvp(pval->flag);
#endif
	
	// 単体
	if ( ( pval->len[1] == 1 && pval->len[2] == 0 )
#if defined(with_Vector) || defined(with_Array)
		|| ( pval->flag >= HSPVAR_FLAG_USERDEF && (false
#ifdef with_Vector
			|| !strcmp(hvp->vartype_name, vector_vartype_name)
#endif
#ifdef with_Array
			|| !strcmp(hvp->vartype_name, array_vartype_name)
#endif
		))
#endif
	) {
		addVarScalar(pval, 0);
		
	// 配列
	} else {
		addVarArray( pval );
	}
	return;
}

//------------------------------------------------
// [add][item] 単体変数
// 
// @ 一つの要素の値を出力する。
//------------------------------------------------
void CVarinfoLine::addVarScalar( PVal const* pval )
{
	addValue(pval->flag, hpimod::PVal_getPtr(pval));
	return;
}

void CVarinfoLine::addVarScalar(PVal const* pval, APTR aptr)
{
	addValue(pval->flag, hpimod::PVal_getPtr(pval, aptr));
	return;
}

//------------------------------------------------
// [add][item] 標準配列変数
//------------------------------------------------
void CVarinfoLine::addVarArray( PVal const* pval )
{
	auto const hvp = hpimod::getHvp(pval->flag);

	// cntElems[1 + i] = 部分i次元配列の要素数 (例えば配列 int(2, 3, 4) だと、cntElems = {1, 2, 2*3, 2*3*4, 0})
	size_t cntElems[1 + hpimod::ArrayDimMax] = { 1 };
	size_t cntDim = 0;
	for ( size_t i = 0; i < hpimod::ArrayDimMax; ++ i ) {
		if ( pval->len[i + 1] > 0 ) {
			cntElems[i + 1] = pval->len[i + 1] * cntElems[i];	
			cntDim  ++;
		} else {
			break;
		}
	}

	cat(strf("<%s>", hvp->vartype_name));
	if ( cntDim == 0 ) {
		cat("[]");
	} else {
		addVarArrayRec(pval, cntElems, cntDim - 1, 0);
	}
	return;
}

void CVarinfoLine::addVarArrayRec(PVal const* pval, size_t const cntElems[], size_t idxDim, APTR aptr_offset)
{
	cat("[");
	for ( int i = 0; i < pval->len[idxDim + 1]; ++i ) {
		if ( i != 0 ) cat(", ");
		// 2次以上 => 配列を出力
		if ( idxDim >= 1 ) {
			addVarArrayRec(pval, cntElems, idxDim - 1, aptr_offset + (i * cntElems[idxDim]));

		// 1次 => 各要素を出力
		} else {
			addVarScalar(pval, aptr_offset + i);
		}
	}
	cat("]");
	return;
}

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVarinfoLine::addItem_flexValue( FlexValue const* fv )
{
	assert(fv);
	 if ( !fv->ptr || fv->type == FLEXVAL_TYPE_NONE ) {
		cat( "(nullmod)" );

	} else {
		auto const stdat = hpimod::FlexValue_getModule(fv);
		cat(strf("<%s>{", makeModuleClassNameString(stdat, hpimod::FlexValue_isClone(fv)).c_str()));
		addPrmstack(stdat, fv->ptr);
		cat("}" );
	}
}

//------------------------------------------------
// [add][item] prmstack
//------------------------------------------------
void CVarinfoLine::addPrmstack(stdat_t stdat, void const* prmstack)
{
	int prev_mptype = MPTYPE_NONE;
	int i = 0;		// structtag でない要素の個数を数える
	std::for_each(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat), [&](STRUCTPRM const& stprm) {
		auto const member = hpimod::Prmstack_getMemberPtr(prmstack, &stprm);
		if ( i != 0 ) {
			// 最初の local の前には空白を1つ多めに置く
			cat((prev_mptype != MPTYPE_LOCALVAR && stprm.mptype == MPTYPE_LOCALVAR)
				? ",  " : ", ");
		}
		addParameter(stdat, &stprm, member);

		if ( stprm.mptype != MPTYPE_STRUCTTAG ) ++i;
		prev_mptype = stprm.mptype;
	});
	return;
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVarinfoLine::addParameter(stdat_t stdat, stprm_t stprm, void const* member)
{
	switch ( stprm->mptype ) {
		case MPTYPE_STRUCTTAG:
		//	cat( hpimod::STRUCTDAT_getName(stdat) );
			break;

		// 変数 (PVal*)
		//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			cat("& ");		// 参照であることを示す (var か array かは明らか)
			auto const vardata = cptr_cast<MPVarData*>(member);

			if ( stprm->mptype == MPTYPE_SINGLEVAR ) {
				addVarScalar(vardata->pval, vardata->aptr);
			} else {
				addVar(vardata->pval);
			}
			break;
		}
		// 変数 (PVal)
		case MPTYPE_LOCALVAR:
		{
			auto const pval = cptr_cast<PVal*>(member);
			addVar(pval);
			break;
		}
		// thismod
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
			addItem_flexValue(Sysvar_getThismod());
			break;
		/*{
			auto const thismod = cptr_cast<MPModVarData*>(member);
			PVal* const pvSelf = thismod->pval;
			pvSelf->offset = thismod->aptr;

			auto const hvp = hpimod::getHvp(pvSelf->flag);
			auto const fv = cptr_cast<FlexValue*>(hvp->GetPtr(pvSelf));
			addItem_flexValue(fv);
			break;
		}//*/
		// 文字列 (char**)
	//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
			addValue(HSPVAR_FLAG_STR, *cptr_cast<char**>(member));
			break;

		// その他
		case MPTYPE_DNUM:
			addValue(HSPVAR_FLAG_DOUBLE, cptr_cast<double*>(member));
			break;

		case MPTYPE_INUM:
			addValue(HSPVAR_FLAG_INT, cptr_cast<int*>(member));
			break;

		case MPTYPE_LABEL:
			addValue(HSPVAR_FLAG_LABEL, cptr_cast<label_t*>(member));
			break;

		// 他 => 無視
		default:
			cat(strf("(ignored: %d)", stprm->mptype));
			break;
	}
	return;
}

# if 0
# include <windows.h>
# include <stdio.h>
# include <stdarg.h>
# define DbgArea /* empty */
# define dbgout(message, ...) msgboxf<void>(message, __VA_ARGS__)//MessageBoxA(0, message, "hpi", MB_OK)
template<class T>
T msgboxf(char const* sFormat, ...)
{
	static char stt_buffer[1024];
	va_list arglist;
	va_start( arglist, sFormat );
	vsprintf_s( stt_buffer, 1024 - 1, sFormat, arglist );
	va_end( arglist );
	MessageBoxA( nullptr, stt_buffer, "hpi", MB_OK );
}
# endif
#ifdef with_Assoc
//------------------------------------------------
// [add][item] assoc
//------------------------------------------------
void CVarinfoLine::addItem_assoc(CAssoc* src)
{
	if ( !src ) {
		cat("(null assoc)");
		return;
	}

	auto const hvp = hpimod::seekHvp(assoc_vartype_name);
	StAssocMapList* const head = (reinterpret_cast<GetMapList_t>(hvp->user))(src);

	// 要素なし
	if ( !head ) {
		cat("(empty assoc)");
		return;
	}

	// 全キーのリスト
	cat("[");
	{
		// 列挙
		for ( StAssocMapList* list = head; list != nullptr; list = list->next ) {
			if ( list != head ) cat(", ");
			addValue(HSPVAR_FLAG_STR, list->key);
			cat(": ");
			addVar(list->pval);
		//	dbgout("%p: key = %s, pval = %p, next = %p", list, list->key, list->pval, list->next );
		}

		// リストの解放
		for ( StAssocMapList* list = head; list != nullptr; ) {
			StAssocMapList* const next = list->next;
			exinfo->HspFunc_free(list);
			list = next;
		}
	}
	cat("]");
	return;
}
#endif

#ifdef with_Vector
//------------------------------------------------
// [add][item] vector
//------------------------------------------------
void CVarinfoLine::addItem_vector( CVector* src )
{
	if ( !src ) {
		cat( "(null vector)" );
		return;
	}
	
	auto const hvp = hpimod::seekHvp(vector_vartype_name);
	int len;
	PVal** const pvals = (reinterpret_cast<GetVectorList_t>(hvp->user))( src, &len );
	
	// 要素なし
	if ( !pvals ) {
		cat( "(empty vector)" );
		return;
	}
	
	// 全キーのリスト
	cat("[");
	{
		// 列挙
		for ( int i = 0; i < len; ++ i ) {
			if ( i != 0 ) cat( ", " );
			addVar( pvals[i] );
		//	dbgout("%p: idx = %d, pval = %p, next = %p", list, idx, list->pval, list->next );
		}
	}
	cat("]");
	return;
}
#endif

#ifdef with_Array
//------------------------------------------------
// [add][item] array
//------------------------------------------------
void CVarinfoLine::addItem_array( CArray* src )
{
	if ( !src ) {
		cat( "(null array)" );
		return;
	}
	
	auto const hvp = hpimod::seekHvp(array_vartype_name);
	PVal* const pvInner = (reinterpret_cast<GetArray_t>(hvp->user))(src);
	
	// 要素なし
	if ( !pvInner || pvInner->len[1] == 0 ) {
		cat( "(empty array)" );
		return;
	}
	
	// 表示
	addVarArray( pvInner );
	return;
}
#endif

//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// [add] モジュール外観
//------------------------------------------------
void CVarinfoLine::addModuleOverview(char const* name, CVarTree::ModuleNode const& tree)
{
	catln(strf("[%s]", name));

	for ( auto const& iter : tree ) {
		//for ( auto iter = tree.begin(); iter != tree.end(); ++iter ) {
		CVarTree const& child = *iter.second;

		child.match<void>([&](CStaticVarTree::VarNode const& child) {
			auto const varname = child.getName();
			auto const varname_raw = removeScopeResolution(varname);

			cat(strf("%s\t= ", varname_raw.c_str()));
			addVar(hpimod::seekSttVar(varname.c_str()));
			catCrlf();

		}, [&](CStaticVarTree::ModuleNode const& child) {
			// (入れ子の)モジュールは名前だけ表示しておく
			catln(child.getName());
		});
	}
	return;
}

//------------------------------------------------
// [add] システム変数外観
//------------------------------------------------
void CVarinfoLine::addSysvarsOverview()
{
	catln("[システム変数]");
	
	for ( int i = 0; i < SysvarCount; ++i ) {
		cat(strf("%s\t= ", SysvarData[i].name));
		addSysvar(static_cast<SysvarId>(i));
		catCrlf();
	}
	return;
}

#ifdef with_WrapCall
using namespace WrapCall;

//------------------------------------------------
// [add] 呼び出し外観
// 
// depends on WrapCall
//------------------------------------------------
void CVarinfoLine::addCallsOverview(ResultNodeData const* pLastResult)
{
	catln("[呼び出し履歴]");

	auto const range = WrapCall::getCallInfoRange();
	std::for_each(range.first, range.second, [&](stkCallInfo_t::value_type const& pCallInfo) {
		addCall(pCallInfo->stdat, pCallInfo->getPrmstk());
		catCrlf();
	});

	// 最後の返値
	if ( pLastResult ) {
		auto const result = "-> " + pLastResult->valueString;
		catln(result);
	}
	return;
}
#endif

//**********************************************************
//        下請け関数
//**********************************************************
#if 0
//------------------------------------------------
// 改行を連結する
//------------------------------------------------
void CVarinfoLine::cat_crlf( void )
{
	if ( mlenLimit < 2 ) return;
	
	mpBuf->append( "\r\n" );
	mlenLimit -= 2;
	return;
}

//------------------------------------------------
// 文字列を連結する
//------------------------------------------------
void CVarinfoLine::cat( char const* src )
{
	if ( mlenLimit <= 0 ) return;
	
	cat( src, strlen(src) );
	return;
}

void CVarinfoLine::cat( char const* src, size_t len )
{
	if ( static_cast<int>(len) > mlenLimit ) {		// 限界なら
		mpBuf->append( src, mlenLimit );
		mpBuf->append( "(長すぎたので省略しました。)" );
		mlenLimit  = 2;
	} else {
		mpBuf->append( src );
		mlenLimit -= len;
	}
	return;
}

//------------------------------------------------
// 書式付き文字列を連結する
//------------------------------------------------
void CVarinfoLine::catf( char const* format, ... )
{
	if ( mlenLimit <= 0 ) return;
	
	va_list   arglist;
	va_start( arglist, format );
	
	string stmp( vstrf( format, arglist ) );
	size_t   len( stmp.length() );
	
	cat( stmp.c_str(), len );
	
	va_end( arglist );
	return;
}
#endif
