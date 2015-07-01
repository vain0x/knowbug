// 変数データのツリー形式文字列

#include <algorithm>
#include "module/ptr_cast.h"

#include "main.h"

#include "CVarinfoTree.h"
#include "SysvarData.h"

#ifdef with_Assoc
# include "hpimod/var_assoc/for_knowbug.var_assoc.h"
#endif
#ifdef with_Vector
# include "hpimod/var_vector/for_knowbug.var_vector.h"
#endif
#ifdef with_Array
# include "hpimod/var_array/src/for_knowbug.var_array.h"
#endif

#include "with_Script.h"
#include "with_ModPtr.h"

extern string getVartypeString(PVal const* pval);

string removeScopeResolution(string const& name);
string getStPrmName(stprm_t stprm, int idx);

//------------------------------------------------
// 構築
//------------------------------------------------
CVarinfoTree::CVarinfoTree(int lenLimit )
	: mBuf( lenLimit )
{
	mBuf.reserve(0x100);
	return;
}

//##########################################################
//        ルート処理関数
//##########################################################
//------------------------------------------------
// [add] システム変数
// 
// @result: メモリダンプするバッファとサイズ
//------------------------------------------------
void CVarinfoTree::addSysvar(SysvarId id)
{
	char const* const name = SysvarData[id].name;

	switch ( id ) {
		case SysvarId_Refstr:
			addValue(name, HSPVAR_FLAG_STR, ctx->refstr);
			break;

		case SysvarId_Refdval:
			addValue(name, HSPVAR_FLAG_DOUBLE, &ctx->refdval);
			break;

		case SysvarId_Cnt:
			if ( int lvLoop = ctx->looplev ) {
				mBuf.catNodeBegin(name);
				for ( ; lvLoop > 0; -- lvLoop ) {
					int const cnt = ctx->mem_loop[lvLoop].cnt;
					mBuf.catLeaf(strf("#%d", lvLoop), strf("%d", cnt).c_str());
				}
				mBuf.catNodeEnd();
			} else {
				mBuf.catLeafExtra(name, "out_of_loop");
			}
			break;

		case SysvarId_NoteBuf:
		{
			PVal* const pval = ctx->note_pval;
			APTR  const aptr = ctx->note_aptr;
			if ( pval && pval->flag == HSPVAR_FLAG_STR ) {
				auto const src = ptr_cast<char*>(hpimod::PVal_getPtr(pval, aptr));
				mBuf.catLeaf(
					strf("%s (0x%08X[%d])", name, address_cast(pval), aptr).c_str(),
					src
				);
			} else {
				mBuf.catLeafExtra(name, "not_exist");
			}
			break;
		}
		case SysvarId_Thismod:
			if ( auto const fv = Sysvar_getThismod() ) {
				addItem_flexValue(name, fv);
			} else {
				mBuf.catLeafExtra(name, "not_exist or null");
			}
			break;
		default:
			// 整数値
			if ( SysvarData[id].type == HSPVAR_FLAG_INT ) {
				addValue(name, HSPVAR_FLAG_INT, Sysvar_getPtrOfInt(id));
				break;
			} else throw;
	};
}

//------------------------------------------------
// [add] 呼び出し
//------------------------------------------------
void CVarinfoTree::addCall(stdat_t stdat, void const* prmstk)
{
	char const* const nameCmd = hpimod::STRUCTDAT_getName(stdat);
	string const name =
		(stdat->index == STRUCTDAT_INDEX_CFUNC
		? strf("%s()", nameCmd)	// 関数なら名前に () をつける
		: nameCmd);
	addCall(name.c_str(), stdat, prmstk);
	return;
}

void CVarinfoTree::addCall(char const* name, stdat_t stdat, void const* prmstk)
{
	if ( !prmstk ) {
		mBuf.catLeafExtra(name, "展開中");
	} else {
		mBuf.catNodeBegin(name);
		addPrmstack(stdat, prmstk);
		mBuf.catNodeEnd();
	}
	return;
}

//------------------------------------------------
// [add] 返値
//------------------------------------------------
void CVarinfoTree::addResult(char const* name, void const* ptr, vartype_t type)
{
	addValue(name, type, ptr);
	return;
}

//##########################################################
//        要素ごとの処理関数
//##########################################################
//------------------------------------------------
// [add][item] 値
//------------------------------------------------
void CVarinfoTree::addValue(char const* name, vartype_t type, void const* ptr)
{
	// ネスト対策
	if ( mBuf.inifiniteNesting() ) {
		mBuf.catLeafExtra(name, "too_many_nesting");
		return;
	}

#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
	auto const vtname = hpimod::getHvp(type)->vartype_name;
#endif
	
	if ( type == HSPVAR_FLAG_STRUCT ) {
		addItem_flexValue(name, cptr_cast<FlexValue*>(ptr));
#ifdef with_Assoc
	// assoc 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, assoc_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CAssoc**>( ptr );
		addItem_assoc(name, src);
#endif
#ifdef with_Vector
	// vector 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, vector_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CVector**>( ptr );
		addItem_vector(name, src);
#endif
#ifdef with_Array
	// array 型
	} else if ( type >= HSPVAR_FLAG_USERDEF && strcmp(vtname, array_vartype_name) == 0 ) {
		auto const src = *cptr_cast<CArray**>( ptr );
		addItem_array(name, src);
#endif
#ifdef with_ModPtr
	} else if ( type == HSPVAR_FLAG_INT && ModPtr::isValid(*cptr_cast<int*>(ptr)) ) {
		auto const modptr = *cptr_cast<int*>(ptr);
		string const name2 = strf("%s = mp#%d", name, ModPtr::getIdx(modptr));
		addItem_flexValue(name2.c_str(), ModPtr::getValue(modptr));
#endif
//	} else if ( type == HSPVAR_FLAG_STR ) {
//		addItem_string( cptr_cast<char*>(ptr) );
		
	} else {
		auto const dbgstr = stringizeSimpleValue( type, ptr, false );
		mBuf.catLeaf(name, dbgstr.c_str() );
	}
	return;
}

//------------------------------------------------
// [add][item] 変数
//------------------------------------------------
void CVarinfoTree::addVar(char const* name, PVal const* pval)
{
#if defined(with_Vector) || defined(with_Array)
	// vector や array は常に単体変数とみなす。
	auto const hvp = hpimod::getHvp(pval->flag);
	if ( pval->flag >= HSPVAR_FLAG_USERDEF && (false
#ifdef with_Vector
			|| !strcmp(hvp->vartype_name, vector_vartype_name)
#endif
#ifdef with_Array
			|| !strcmp(hvp->vartype_name, array_vartype_name)
#endif
		) ) {
		addVarScalar(name, pval);
		return;
	}
#endif

	// 単体
	if ( pval->len[1] == 1 && pval->len[2] == 0 ) {
		addVarScalar(name, pval, 0);
		
	// 配列
	} else {
		addVarArray(name, pval);
	}
	return;
}

//------------------------------------------------
// [add][item] 単体変数
// 
// @ 要素の値を出力する。
//------------------------------------------------
void CVarinfoTree::addVarScalar(char const* name, PVal const* pval)
{
	addValue(name, pval->flag, hpimod::PVal_getPtr(pval));
	return;
}

void CVarinfoTree::addVarScalar(char const* name, PVal const* pval, APTR aptr)
{
	addValue(name, pval->flag, hpimod::PVal_getPtr(pval, aptr));
	return;
}

//------------------------------------------------
// [add][item] 標準配列変数
//------------------------------------------------
void CVarinfoTree::addVarArray(char const* name, PVal const* pval)
{
	mBuf.catNodeBegin(name);

	mBuf.catAttribute("type", getVartypeString(pval).c_str());

	// 各要素を追加する
	size_t const cntElems = hpimod::PVal_cntElems(pval);
	size_t const dim = hpimod::PVal_maxDim(pval);
	int indexes[hpimod::ArrayDimMax] = { 0 };

	for ( size_t i = 0; i < cntElems; ++ i ) {
		
		// aptr を分解して添字を求める
		calcIndexesFromAptr(indexes, i, &pval->len[1], cntElems, dim);
		
		// 要素の値を追加
		string const nameChild = makeArrayIndexString(dim, indexes);
		addVarScalar(nameChild.c_str(), pval, i);
	}

	mBuf.catNodeEnd();
	return;
}

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVarinfoTree::addItem_flexValue(char const* name, FlexValue const* fv)
{
	assert(fv);
	
	if ( !fv->ptr || fv->type == FLEXVAL_TYPE_NONE ) {
		mBuf.catLeafExtra(name, "null");

	} else {
		mBuf.catNodeBegin(name);

	//	cat(strf("%s.myid = %d", getIndent().c_str(), fv->myid ));
		auto const stdat = hpimod::FlexValue_getModule(fv);
		mBuf.catAttribute("modcls",
			makeModuleClassNameString(stdat, hpimod::FlexValue_isClone(fv)).c_str());
		addPrmstack(stdat, fv->ptr);

		mBuf.catNodeEnd();
	}
	return;
}

//------------------------------------------------
// [add][item] prmstack
// 
// @ 中身を出力する。
//------------------------------------------------
void CVarinfoTree::addPrmstack(stdat_t stdat, void const* prmstack)
{
//	mlvNest ++;
//	BaseData baseChild( nullptr, getIndent() );
/*
	mBuf.catAttribute( "id_finfo", strf("%d", stdat->subid).c_str() );
	mBuf.catAttribute( "id_minfo", strf("%d", stdat->prmindex).c_str() );
//*/
	int i = 0;
	std::for_each(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat), [&](STRUCTPRM const& stprm) {
		auto const member = hpimod::Prmstack_getMemberPtr(prmstack, &stprm);

		addParameter(getStPrmName(&stprm, i).c_str(), stdat, &stprm, member);

		// structtag => メンバではないので、数えない
		if ( stprm.mptype != MPTYPE_STRUCTTAG ) { ++i; }
	});
//	mlvNest --;
	return;
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVarinfoTree::addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member)
{
	switch ( stprm->mptype )
	{
		case MPTYPE_STRUCTTAG:
		//	mBuf.catAttribute("modcls", hpimod::STRUCTDAT_getName(hpimod::STRUCTPRM_getStDat(stprm)));
			break;
			
		// 変数 (PVal*)
	//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			auto const vardata = cptr_cast<MPVarData*>(member);
			
			if ( stprm->mptype == MPTYPE_SINGLEVAR ) {
				addVarScalar(name, vardata->pval, vardata->aptr);
			} else {
				addVar(name, vardata->pval);
			}
			break;
		}
		// 変数 (PVal)
		case MPTYPE_LOCALVAR:
		{
			auto const pval = cptr_cast<PVal*>(member);
			addVar(name, const_cast<PVal*>(pval));
			break;
		}
		// thismod
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
			addItem_flexValue(name, Sysvar_getThismod());
			break;
		/*{
			auto const thismod = ptr_cast<MPModVarData*>(member);
			PVal* const pvSelf = thismod->pval;
			pvSelf->offset = thismod->aptr;
			
			auto const hvp = hpimod::getHvp(pvSelf->flag);
			auto const fv = ptr_cast<FlexValue*>(hvp->GetPtr(pvSelf));
			addItem_flexValue(name, fv);
			break;
		}
		//*/
		// 文字列 (char**)
	//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
		{
			addValue(name, HSPVAR_FLAG_STR, *cptr_cast<char**>(member));
			break;
		}
		// その他
		case MPTYPE_DNUM:
			addValue(name, HSPVAR_FLAG_DOUBLE, cptr_cast<double*>(member));
			break;

		case MPTYPE_INUM:
			addValue(name, HSPVAR_FLAG_INT, cptr_cast<int*>(member));
			break;

		case MPTYPE_LABEL:
			addValue(name, HSPVAR_FLAG_LABEL, cptr_cast<label_t*>(member));
			break;

		// 他 => 無視
		default:
			mBuf.catLeaf(name, strf("ignored (mptype = %d)", stprm->mptype).c_str());
			break;
	}
	return;
}

#ifdef with_Assoc
//------------------------------------------------
// [add][item] assoc
//------------------------------------------------
void CVarinfoTree::addItem_assoc(char const* name, CAssoc* src)
{
	if ( !src ) {
		mBuf.catLeafExtra(name, "null assoc");
		return;
	}
	
	auto const hvp = hpimod::seekHvp(assoc_vartype_name);
	StAssocMapList* const head = (reinterpret_cast<GetMapList_t>(hvp->user))( src );
	
	// 要素なし
	if ( !head ) {
		mBuf.catLeafExtra(name, "empty assoc");
		return;
	}
	
	// 全キーのリスト
	mBuf.catNodeBegin(name);
	{
		// 列挙
		for ( StAssocMapList* list = head; list != nullptr; list = list->next ) {
			addVar(list->key, list->pval);
		//	dbgout("%p: key = %s, pval = %p, next = %p", list, list->key, list->pval, list->next );
		}
		
		// リストの解放
		for ( StAssocMapList* list = head; list != nullptr; ) {
			StAssocMapList* const next = list->next;
			exinfo->HspFunc_free( list );
			list = next;
		}
		
	}
	mBuf.catNodeEnd();
	return;
}
#endif

#ifdef with_Vector
//------------------------------------------------
// [add][item] vector
//------------------------------------------------
void CVarinfoTree::addItem_vector(char const* name, CVector* src)
{
	if ( !src ) {
		mBuf.catLeafExtra(name, "null vector");
		return;
	}
	
	auto const hvp = hpimod::seekHvp(vector_vartype_name);
	int len;
	PVal** const pvals = (reinterpret_cast<GetVectorList_t>(hvp->user))( src, &len );
	
	// 要素なし
	if ( !pvals ) {
		mBuf.catLeafExtra(name, "empty vector");
		return;
	}
	
	// 全キーのリスト
	mBuf.catNodeBegin(name);
	{
		mBuf.catAttribute( "length", strf("%d", len).c_str() );
		
		// 列挙
		for ( int i = 0; i < len; ++ i ) {
			addVar(makeArrayIndexString(1, &i).c_str(), pvals[i]);
		//	dbgout("%p: idx = %d, pval = %p, next = %p", list, idx, list->pval, list->next );
		}
		
	}
	mBuf.catNodeEnd();
	return;
}
#endif

#ifdef with_Array
//------------------------------------------------
// [add][item] array
//------------------------------------------------
void CVarinfoTree::addItem_array(char const* name,  CArray* src)
{
	if ( !src ) {
		mBuf.catLeafExtra(name, "null array");
		return;
	}
	
	auto const hvp = hpimod::seekHvp(array_vartype_name);
	PVal* const pvInner = (reinterpret_cast<GetArray_t>(hvp->user))( src );
	
	// 要素なし
	if ( !pvInner || pvInner->len[1] == 0 ) {
		mBuf.catLeafExtra(name, "empty array");
		return;
	}
	
	// 表示
	addVarArray(name, pvInner);
	return;
}
#endif

//------------------------------------------------
// 
//------------------------------------------------

//**********************************************************
//        下請け関数
//**********************************************************
#if 0
//------------------------------------------------
// 改行を連結する
//------------------------------------------------
void CVarinfoTree::cat_crlf( void )
{
	if ( mlenLimit < 2 ) { mlenLimit = 0; return; }
	
	mpBuf->append( "\r\n" );
	mlenLimit -= 2;
	return;
}

//------------------------------------------------
// 文字列を連結する
//------------------------------------------------
void CVarinfoTree::cat( char const* src )
{
	if ( mlenLimit <= 0 ) return;
	
	cat( src, strlen(src) );
	return;
}

void CVarinfoTree::cat( char const* src, size_t len )
{
	if ( static_cast<int>(len) > mlenLimit ) {		// 限界なら
		mpBuf->append( src, mlenLimit );
		mpBuf->append( "(長すぎたので省略しました。)" );
		mlenLimit  = 2;
	} else {
		mpBuf->append( src );
		mlenLimit -= len;
	}
	
	cat_crlf();
	return;
}

//------------------------------------------------
// 書式付き文字列を連結する
//------------------------------------------------
void CVarinfoTree::catf( char const* format, ... )
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

//------------------------------------------------
// 単純な値を文字列化する
//------------------------------------------------
string stringizeSimpleValue( vartype_t type, void const* ptr, bool bShort )
{
	switch ( type ) {
		case HSPVAR_FLAG_STR:
		{
			auto const val = cptr_cast<char*>(ptr);
			return (bShort
				? toStringLiteralFormat(val)
				: string(val));
		}
		case HSPVAR_FLAG_COMOBJ:  return strf("comobj(0x%08X)",  address_cast( *cptr_cast<void**>(ptr) ));
		case HSPVAR_FLAG_VARIANT: return strf("variant(0x%08X)", address_cast( *cptr_cast<void**>(ptr)) );
		case HSPVAR_FLAG_DOUBLE:  return strf( (bShort ? "%f" : "%.16f"), *cptr_cast<double*>(ptr) );
		case HSPVAR_FLAG_INT:
		{
			int const val = *cptr_cast<int*>(ptr);
#ifdef with_ModPtr
			assert(!ModPtr::isValid(val));	// addItem_value で処理されたはず
#endif
			return (bShort
				? strf("%d", val)
				: strf("%-10d (0x%08X)", val, val));
		}
		case HSPVAR_FLAG_LABEL:
		{
			auto const lb = *cptr_cast<label_t*>(ptr);
			int const idx = hpimod::findOTIndex(lb);
			auto const name =
				(idx >= 0 ? g_dbginfo->ax->getLabelName(idx) : nullptr);
			return (name ? strf("*%s", name) : strf("label(0x%08X)", address_cast(lb)));
		}
		case HSPVAR_FLAG_STRUCT:		// 拡張表示あり
			assert(false); throw;
#if 0
		{
			auto const fv = cptr_cast<FlexValue*>( ptr );
			if ( fv->type == FLEXVAL_TYPE_NONE ) {
				return string( "(empty struct)" ); 
			} else {
				return strf( "struct%s(0x%08X; #%d, size:%d)",
					(fv->type == FLEXVAL_TYPE_CLONE ? "&" : ""),
					fv->customid, address_cast(fv->ptr), fv->size, 
				);
			}
		}
#endif
		default:
		{
			auto const vtname = hpimod::getHvp(type)->vartype_name;
			
#ifdef with_ExtraBasics
			// 拡張基本型
			bool bSigned = false;
			
			if ( strcmp(vtname, "bool") == 0 ) {
				static char const* const bool_name[2] = { "false", "true" };
				return bool_name[ *cptr_cast<bool*>( ptr ) ? 1 : 0 ];
				
			// char (signed char とする)
			} else if (
				   ( strcmp(vtname,  "char") == 0 || strcmp(vtname,   "signed_char") == 0 ) && (bSigned = true)
				|| ( strcmp(vtname, "uchar") == 0 || strcmp(vtname, "unsigned_char") == 0 )
			) {
				int const val = bSigned ? *cptr_cast<signed char*>( ptr ) : *cptr_cast<unsigned char*>( ptr );
				return ( val == 0 ) ? string("0 ('\\0')") : strf("%-3d '%c'", static_cast<int>(val), static_cast<char>(val));
				
			// short
			} else if (
				   ( strcmp(vtname,  "short") == 0 || strcmp(vtname,   "signed_short") == 0 ) && (bSigned = true)
				|| ( strcmp(vtname, "ushort") == 0 || strcmp(vtname, "unsigned_short") == 0 )
			) {
				int const val = static_cast<int>( bSigned ? *cptr_cast<signed short*>( ptr ) : *cptr_cast<unsigned short*>( ptr ) );
				return (bShort ? strf("%d", val) : strf("%-6d (0x%04X)", val, static_cast<short>(val)));
				
			// unsigned int
			} else if ( strcmp(vtname, "uint") == 0 || strcmp(vtname, "unsigned_int") == 0 ) {
				auto const val = *cptr_cast<unsigned int*>( ptr );
				return (bShort ? strf("%d", val) : strf( "%-10d (0x%08X)", val, val ));
				
			// long
			} else if (
				   ( strcmp(vtname,  "long") == 0 || strcmp(vtname,   "signed_long") == 0 ) && (bSigned = true)
				|| ( strcmp(vtname, "ulong") == 0 || strcmp(vtname, "unsigned_long") == 0 ) 
			) {
				auto const   signed_val = *cptr_cast<long long*>(ptr);
				auto const unsigned_val = *cptr_cast<unsigned long long*>(ptr);
				return (bShort
					? strf("%d", (bSigned ? signed_val : unsigned_val))
					: strf("%d (0x%16X)", (bSigned ? signed_val : unsigned_val), signed_val)
				);
			// おまけ
			} else if ( strcmp(vtname, "tribyte") == 0 ) {
				auto const bytes = cptr_cast<char*>( ptr );
				int const val = bytes[0] << 16 | bytes[1] << 8 | bytes[2];
				return (bShort
					? strf("%d", val)
					: strf( "%-8d (0x%06X)", val, val)
				);
			}
#endif
#ifdef with_Modcmd
			// "modcmd_k" 型
			if ( strcmp(vtname, "modcmd_k") == 0 ) {
				int const modcmd = *cptr_cast<int*>(ptr);
				return strf("modcmd(%s)",
					(modcmd == 0xFFFFFFFF) ? "" : hpimod::STRUCTDAT_getName(hpimod::getSTRUCTDAT(modcmd))
				);
			}
#endif
			return strf( "unknown<%s>(0x%08X)", vtname, address_cast( ptr ) );
		}
	}
}

//------------------------------------------------
// 文字列を文字列リテラルの形式に変換する
//------------------------------------------------
string toStringLiteralFormat( char const* src )
{
	size_t const maxlen = (std::strlen(src) * 2) + 2;
	char* const buf = exinfo->HspFunc_malloc(maxlen + 1);
	size_t idx = 0;
	
	buf[idx ++] = '\"';
	
	for ( int i = 0; ; ++ i ) {
		char c = src[i];
		
		// エスケープ・シーケンスを解決する
		if ( c == '\0' ) {
			break;
			
		} else if ( c == '\\' || c == '\"' ) {
			buf[idx++] = '\\';
			buf[idx++] = c;
			
		} else if ( c == '\t' ) {
			buf[idx++] = '\\';
			buf[idx++] = 't';
			
		} else if ( c == '\r' || c == '\n' ) {
			buf[idx++] = '\\';
			
			if ( c == '\r' && src[i + 1] == '\n' ) {	// CR + LF
				buf[idx++] = 'n';
				i ++;
			} else {
				buf[idx++] = 'r';
			}
			
		} else {
			buf[idx++] = c;
		}
	}
	
	buf[idx++] = '\"';
	buf[idx++] = '\0';
	
	string const sResult = buf;
	exinfo->HspFunc_free( buf );
	return std::move(sResult);
}

//------------------------------------------------
// モジュールクラス名を表す文字列
//------------------------------------------------
string makeModuleClassNameString(stdat_t stdat, bool bClone)
{
	auto const modclsName = hpimod::STRUCTDAT_getName(stdat);
	return (bClone
		? strf("%s&", modclsName)
		: string(modclsName));
}

//------------------------------------------------
// 配列添字の文字列
//------------------------------------------------
string makeArrayIndexString(size_t dim, int const indexes[])
{
	switch (dim) {
		case 0: return BracketIdxL BracketIdxR;
		case 1: return strf(BracketIdxL "%d"             BracketIdxR, indexes[0]);
		case 2: return strf(BracketIdxL "%d, %d"         BracketIdxR, indexes[0], indexes[1]);
		case 3: return strf(BracketIdxL "%d, %d, %d"     BracketIdxR, indexes[0], indexes[1], indexes[2]);
		case 4: return strf(BracketIdxL "%d, %d, %d, %d" BracketIdxR, indexes[0], indexes[1], indexes[2], indexes[3]);
		default:
			return "(invalid index)";
	}
}

//------------------------------------------------
// APTR値から添字を計算する
// 
// @prm indexes: dim 個以上の要素を持つ。dim 個より後の要素は変更されない。
//------------------------------------------------
void calcIndexesFromAptr(int* indexes, APTR aptr, int const* lengths, size_t cntElems, size_t dim)
{
	for ( size_t idxDim = 0; idxDim < dim; ++idxDim ) {
		indexes[idxDim] = aptr % lengths[idxDim];
		aptr /= lengths[idxDim];
	}
	return;
}

//------------------------------------------------
// スコープ解決を取り除いた名前
//------------------------------------------------
string removeScopeResolution(string const& name)
{
	int const idxScopeResolution = name.find('@');
	return (idxScopeResolution != string::npos
		? name.substr(0, idxScopeResolution)
		: name);
}

//------------------------------------------------
// 構造体パラメータの名前
// 
// デバッグ情報から取得する。なければ「(idx)」とする。
//------------------------------------------------
string getStPrmName(stprm_t stprm, int idx)
{
	int const subid = hpimod::findStPrmIndex(stprm);
	if ( subid >= 0 ) {
		if ( auto const name = g_dbginfo->ax->getPrmName(subid) ) {
			return removeScopeResolution(name);
		}
	}
	return makeArrayIndexString(1, &idx);
}
