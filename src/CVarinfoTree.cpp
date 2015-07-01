// 変数データのツリー形式文字列

#include <algorithm>
#include "module/mod_cast.h"

#include "CVarinfoTree.h"
#include "SysvarData.h"

#ifdef clhsp
# include "hsp3/mod_vector.h"
#endif
#ifdef with_Assoc
# include "D:/Docs/prg/cpp/MakeHPI/hpimod/var_assoc/for_knowbug.var_assoc.h"	// あまりにも遠いのでフルパス
#endif
#ifdef with_Vector
# include "D:/Docs/prg/cpp/MakeHPI/hpimod/var_vector/for_knowbug.var_vector.h"	// あまりにも遠いのでフルパス
#endif
#ifdef with_Array
# include "D:/Docs/prg/cpp/MakeHPI/var_array/src/for_knowbug.var_array.h"	// あまりにも遠いのでフルパス
#endif

#include "with_Script.h"
#include "with_ModPtr.h"

static const char* getMPTypeString( int mptype );

//##############################################################################
//                定義部 : CVarinfoTree
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CVarinfoTree::CVarinfoTree( DebugInfo& dbginfo, int lenLimit )
	: mdbginfo ( dbginfo )
	, mpBuf    ( new CString )
	, mlvNest  ( 0 )
	, mlenLimit( lenLimit - 30 )	// 「長すぎたので省略しました」を出力するため、余裕を持たせる
{
	mpBuf->reserve( std::min(0x100, mlenLimit + 1) );
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CVarinfoTree::~CVarinfoTree()
{
	delete mpBuf; mpBuf = nullptr;
	mlvNest = 0;
	return;
}

//##########################################################
//        ルート処理関数
//##########################################################
//------------------------------------------------
// [add] 変数
//------------------------------------------------
void CVarinfoTree::addVar( PVal* pval, const char* name )
{
	BaseData base ( name, getIndent() );
	
	addItem_var( base, pval );
	return;
}

#if 0
//------------------------------------------------
// [add] 単体変数
//------------------------------------------------
void CVarinfoTree::addVarScalar( PVal* pval, const char* name )
{
	BaseData base ( name, getIndent() );
	
	addItem_varScalar( base, pval );
	return;
}
#endif

#ifdef clhsp
//------------------------------------------------
// [add] modinst
//------------------------------------------------
void CVarinfoTree::addModInst( ModInst* mv, const char* name )
{
	BaseData base ( name, getIndent() );
	addItem_modinst( base, mv );
	return;
}

#else

//------------------------------------------------
// [add] flexValue
//------------------------------------------------
void CVarinfoTree::addFlexValue( FlexValue* fv, const char* name )
{
	BaseData base ( name, getIndent() );
	addItem_flexValue( base, fv );
	return;
}
#endif

//------------------------------------------------
// [add] システム変数
//------------------------------------------------
void CVarinfoTree::addSysvar( int idx, const char* name, void** ppDumped, size_t* pSizeToDump )
{
	BaseData base( name, getIndent() );
	switch ( idx ) {
		// 整数値
		case SysvarId_Stat:
		case SysvarId_StrSize:
		case SysvarId_Looplev:
		case SysvarId_Sublev:
		case SysvarId_Err:
	//	case SysvarId_MouseX:
	//	case SysvarId_MouseY:
	//	case SysvarId_MouseW:
		{
			int* p;
			switch ( idx ) {
				case SysvarId_Stat:    p = &mdbginfo.ctx->stat;    break;
				case SysvarId_StrSize: p = &mdbginfo.ctx->strsize; break;
				case SysvarId_Looplev: p = &mdbginfo.ctx->looplev; break;
				case SysvarId_Sublev:  p = &mdbginfo.ctx->sublev;  break;
				case SysvarId_Err:     p = ptr_cast<int*>( &mdbginfo.ctx->err ); break;
			}
			addItem_value( base, HSPVAR_FLAG_INT, p );
			break;	// no dump
		}
		// refstr
		case SysvarId_Refstr:
		{
			char*& refstr = mdbginfo.ctx->refstr;
			addItem_value( base, HSPVAR_FLAG_STR, refstr );
			*ppDumped    = refstr;
			*pSizeToDump = HSPCTX_REFSTR_MAX;
			break;
		}
		// refdval
		case SysvarId_Refdval:
		{
			double& dval = mdbginfo.ctx->refdval;
			addItem_value( base, HSPVAR_FLAG_DOUBLE, &dval );
			*ppDumped    = &dval;
			*pSizeToDump = sizeof(dval);
			break;
		}
		// cnt
		case SysvarId_Cnt:
		{
			int lvLoop ( mdbginfo.ctx->looplev );
			if ( lvLoop == 0 ) {
				cat( "cnt = (out of loop)" );
			} else {
				cat( "cnt:" );
				
				for ( ; lvLoop > 0; -- lvLoop ) {
#ifdef clhsp
					int& cnt = mdbginfo.ctx->mem_loop[lvLoop].cnt;
#else
					int& cnt = ptr_cast<LOOPDAT*>( &mdbginfo.ctx->mem_loop )[lvLoop].cnt;
#endif
					catf( "\t#%d = %d", lvLoop, cnt );
				}
			}
			break;	// no dump
		}
		// (params)
		case SysvarId_Params:
		{
			static const char* stt_name_params[] = { "iparam", "wparam", "lparam" };
			const int params[] = {
				mdbginfo.ctx->iparam,
				mdbginfo.ctx->wparam,
				mdbginfo.ctx->lparam
			};
			for ( int i = 0; i < (sizeof(params) / sizeof(params[0])); ++ i ) {
				catf( "%s = %-10d (0x%08X)", stt_name_params[i], params[i], params[i] );
			}
			break;	// no dump
		}
		// (notebuf)
		case SysvarId_NoteBuf:
		{
			PVal* pval = mdbginfo.ctx->note_pval;
			APTR  aptr = mdbginfo.ctx->note_aptr;
			if ( pval ) {
				HspVarProc* pHvp = mdbginfo.exinfo->HspFunc_getproc(HSPVAR_FLAG_STR);
				pval->offset = aptr;
				char* src  = ptr_cast<char*>( pHvp->GetPtr(pval) );
				catf( "[notebuf] (0x%08X[%d]) {\r\n%s\r\n}", address_cast(pval), aptr, src );
				*ppDumped    = src;
				*pSizeToDump = pHvp->GetSize( ptr_cast<PDAT*>(src) );
			} else {
				cat( "notebuf = (un-used)" );
			}
			break;
		}
		// thismod
		case SysvarId_Thismod:
		{
			if ( mdbginfo.ctx->prmstack ) {
				MPThismod* thismod = ptr_cast<MPThismod*>( mdbginfo.ctx->prmstack );
				
				if ( thismod->magic == MODVAR_MAGICCODE ) {
#ifdef clhsp
					addItem_modinst( base, thismod->mv );
#else
					PVal* pval( thismod->pval );
					pval->offset = thismod->aptr;
					
					HspVarProc* pHvp( mdbginfo.exinfo->HspFunc_getproc(pval->flag) );
					FlexValue* fv = ptr_cast<FlexValue*>(pHvp->GetPtr(pval));
					addItem_flexValue( base, fv );
					*ppDumped    = fv->ptr;
					*pSizeToDump = fv->size;
#endif
					break;
				}
			}
			cat( "thismod = (nullmod or un-used)" );
			break;
		}
		/*
		// ginfo
		case SysvarId_GInfo:
		{
			cat( "(未実装)" );
			break;
		}
		//*/
		/*
		// dirinfo
		case SysvarId_DirInfo:
		{
			cat( "(未実装)" );
			break;
		}
		//*/
	};
	return;
}

//------------------------------------------------
// [add] 呼び出し
//------------------------------------------------
void CVarinfoTree::addCall( STRUCTDAT* stdat, void* prmstk, const char* name )
{
	BaseData base ( name, getIndent() );
	
	catf(
		"%s%s%s:", base.getIndent(), name,
		(stdat->index == STRUCTDAT_INDEX_CFUNC ? "()" : "")		// 関数なら () をつける
	);
	
	// 実引数
	if ( !prmstk ) {
		mlvNest ++;
		catf("%s([展開中])", getIndent());
		mlvNest --;
	} else {
		addItem_prmstack( base, stdat, &mdbginfo.ctx->mem_minfo[stdat->prmindex], prmstk );
	}
	return;
}

//------------------------------------------------
// [add] 返値
//------------------------------------------------
void CVarinfoTree::addResult( void* ptr, vartype_t type, const char* name )
{
	BaseData base( name, getIndent() );
	
	addItem_value( base, type, ptr );
	return;
}

//##########################################################
//        要素ごとの処理関数
//##########################################################
//------------------------------------------------
// [add][item] 値
//------------------------------------------------
void CVarinfoTree::addItem_value( const BaseData& base, vartype_t type, void* ptr )
{
	const char* const vtname = mdbginfo.exinfo->HspFunc_getproc(type)->vartype_name;
	
	if ( type == HSPVAR_FLAG_STRUCT ) {
#ifdef clhsp
		addItem_modinst( base, *ptr_cast<ModInst**>(ptr) );
#else
		addItem_flexValue( base, ptr_cast<FlexValue*>(ptr) );
#endif
#ifdef with_Assoc
	// "assoc_k" 型
	} else if ( strcmp(vtname, "assoc_k") == 0 ) {
		CAssoc* src = *ptr_cast<CAssoc**>( ptr );
		addItem_assoc( base, src );
#endif
#ifdef with_Vector
	// "vector_k" 型
	} else if ( strcmp(vtname, "vector_k") == 0 ) {
		CVector* src = *ptr_cast<CVector**>( ptr );
		addItem_vector( base, src );
#endif
#ifdef with_Array
	// "array_k" 型
	} else if ( strcmp(vtname, "array_k") == 0 ) {
		CArray* src = *ptr_cast<CArray**>( ptr );
		addItem_array( base, src );
#endif
#ifdef with_ModPtr
	} else if ( type == HSPVAR_FLAG_INT && ModPtr::isValid(*ptr_cast<int*>(ptr)) ) {
		CString name = strf( "%s = mp#%d", base.getName(), ModPtr::getIdx(*ptr_cast<int*>(ptr)) );
		BaseData base2( name.c_str(), base.getIndent() );
		addItem_flexValue( base2, ModPtr::getValue(*ptr_cast<int*>(ptr)) );
#endif
//	} else if ( type == HSPVAR_FLAG_STR ) {
//		addItem_string( base, ptr_cast<char*>(ptr) );
		
	} else {
#ifdef clhsp
		char* p = mdbginfo.debug->dbg_toString( type, ptr );
		
		catf( "%s%s = %s", base.getIndent(), base.getName(), p );
		
		mdbginfo.debug->dbg_close( p );
#else
		catf( "%s%s = %s", base.getIndent(), base.getName(), getDbgString( type, ptr ).c_str() );
#endif
	}
	return;
}

//------------------------------------------------
// [add][item] 変数
//------------------------------------------------
void CVarinfoTree::addItem_var( const BaseData& base, PVal* pval )
{
	HspVarProc* pHvp( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	int       length( PValLength( pHvp, pval, 1 ) );
	
	// 単体
	if ( ( length == 1 && PValLength(pHvp, pval, 2) == 0 )
#if clhsp
		&& pval->flag != HSPVAR_FLAG_VECTOR
#endif
#if defined(with_Vector) || defined(with_Array)
		|| ( pval->flag >= HSPVAR_FLAG_USERDEF
			&& (!strcmp(pHvp->vartype_name, "vector_k") || !strcmp(pHvp->vartype_name, "array_k")) )
#endif
	) {
		addItem_varScalar( base, pval );
		
	// 空
	} else if ( length == 0 ) {
		catf( "%s%s (empty)", base.getIndent(), base.getName() );
		
	// 配列
	} else {
		catf( "%s%s:", base.getIndent(), base.getName() );
		
		mlvNest ++;
		
		if ( mlvNest > 512 ) {
			catf( "%s[Error] (too many nesting)", getIndent() );
		} else
#ifdef clhsp
		if ( pval->flag == HSPVAR_FLAG_VECTOR ) {
			addItem_vector( base, ptr_cast<Vector*>( pval->pt ), length );
		} else
#endif
		{
			addItem_varArray( base, pval );
		}
		
		mlvNest --;
	}
	
	return;
}

//------------------------------------------------
// [add][item] 単体変数
// 
// @ 要素の値を出力する。
//------------------------------------------------
void CVarinfoTree::addItem_varScalar( const CVarinfoTree::BaseData& base, PVal* pval )
{
	HspVarProc* pHvp( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	
	addItem_value( base, pval->flag, pHvp->GetPtr(pval) );
	return;
}

void CVarinfoTree::addItem_varScalar( const BaseData& base, PVal* pval, APTR aptr )
{
	int offset_bak = pval->offset;
	pval->offset = aptr;
	addItem_varScalar( base, pval );
	pval->offset = offset_bak;
	return;
}

//------------------------------------------------
// [add][item] 配列変数 (array)
// 
// @ vector 型は来ない。
// @prm base : 子要素(配列変数)のある深さ
//------------------------------------------------
void CVarinfoTree::addItem_varArray( const CVarinfoTree::BaseData& base, PVal* pval )
{
	BaseData baseChild( nullptr, getIndent() );
	
	HspVarProc* pHvp( mdbginfo.exinfo->HspFunc_getproc(pval->flag) );
	
	size_t length[ArrayDimMax] = { 0 };
	uint cntElem( 0 );
	uint maxdim ( 0 );
	
	// 全要素数と最大次元を調べる
	for ( int i = 0; i < ArrayDimMax; ++ i ) {
		length[i] = PValLength( pHvp, pval, i + 1 );
		
		if ( length[i] > 0 ) {
			if ( i == 0 ) cntElem = 1;
			cntElem *= length[i];
			maxdim  ++;
		} else {
			break;
		}
	}
	
	// 配列情報
	catf( "%s.vartype = %s", baseChild.getIndent(), pHvp->vartype_name );
	
	if ( maxdim <= 1 ) {
		catf( "%s.length  = %d", baseChild.getIndent(), length[0] );
	} else {
		catf( "%s.format  = %s (%d in total)",
			baseChild.getIndent(), makeArrayIndexString(maxdim, length).c_str(), cntElem
		);
	}

	// 各要素を追加する
	int offset_bak = pval->offset;			// offset の値を保存しておく
	for ( uint i = 0; i < cntElem; ++ i ) {
		
		// aptr を分解して添字を求める
		APTR aptr ( i );
		uint idx[ArrayDimMax] = { 0 };
		for ( uint idxDim = 0; idxDim < maxdim; ++ idxDim ) {
			idx[idxDim] = aptr % length[idxDim];
			aptr        = aptr / length[idxDim];
		}
		
		CString const sName = makeArrayIndexString(maxdim, idx);
		baseChild.name = sName.c_str();
		
		// 要素の値を追加
		pval->offset = i;
		addItem_value( baseChild, pval->flag, pHvp->GetPtr(pval) );
	}
	pval->offset = offset_bak;
	
	return;
}

//------------------------------------------------
// [add][item] 変数 (vartype: str)
//------------------------------------------------
void CVarinfoTree::addItem_varStr( const BaseData& base, PVal* pval, bool bScalar )
{
	if ( bScalar ) {
		catf( "%s", ptr_cast<char*>( pval->pt ) );
		
	} else {
		addItem_varArray( base, pval );
	}
	return;
}

#ifdef clhsp
//------------------------------------------------
// [add][item] vector
//------------------------------------------------
void CVarinfoTree::addItem_vector( const BaseData& base, Vector* vec, int length )
{
//	CString sName;
//	BaseData baseChild( nullptr, getIndent() );
	
	for ( int i = 0; i < length; ++ i ) {
	//*
		addItem_vecelem( base, vec->list->at(i), i );
	/*/
		PVal* pvElem = vec->list->at( i )->pval;
		
		sName = strf( "[%d]", i );
		baseChild.name = sName.c_str();
		
		// 再帰する
		addItem_var( baseChild, pvElem );
	//*/
	}
	return;
}

//------------------------------------------------
// [add][item] vecelem
//------------------------------------------------
void CVarinfoTree::addItem_vecelem( const BaseData& base, VecElem* vecelem, int idx )
{
	PVal* pval ( vecelem->pval );
	APTR  aptr ( vecelem->aptr );
	
	CString sName(
		strf( "[%d]%s", idx, (aptr < 0) ? "*" : "&" )
	);
	
	mlvNest ++;
	
	const char* pName  ( sName.c_str() );
	BaseData  baseChild( pName, getIndent() );
	const char* pIndent( baseChild.getIndent() );
	
	if ( aptr < 0 || aptr == 0 ) {
		addItem_var( baseChild, pval );
	} else {
		pval->offset = aptr;
		addItem_varScalar( baseChild, pval );
	}
	
	mlvNest --;
	
	return;
}

//------------------------------------------------
// [add][item] modinst
//------------------------------------------------
void CVarinfoTree::addItem_modinst( const BaseData& base, ModInst* mv )
{
	if ( !mv ) {
		catf( "%s%s = (nullmod)", base.getIndent(), base.getName() );
		return;
	}
	
	catf( "%s%s:", base.getIndent(), base.getName() );
	
	STRUCTPRM* stprm ( &mdbginfo.ctx->mem_minfo[mv->id_minfo] );
	STRUCTDAT* stdat ( &mdbginfo.ctx->mem_finfo[stprm->subid] );
	void*      prmstack ( mv->members );
	
	addItem_prmstack( base, stdat, stprm, prmstack );
	
	return;
}

#else

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVarinfoTree::addItem_flexValue( const BaseData& base, FlexValue* fv )
{
	if ( !fv ) {
		catf( "%s[error] (addItem_flexValue) fv = nullptr", base.getIndent() );
		return;
	}
	
	if ( !fv->ptr || fv->type == FLEXVAL_TYPE_NONE ) {
		catf( "%s%s = (empty struct)", base.getIndent(), base.getName() );
		return;
	}
	
	catf( "%s%s:", base.getIndent(), base.getName() );
	
	{
		mlvNest ++;
		if ( fv->type == FLEXVAL_TYPE_CLONE ) {
			catf( "%s@clone", getIndent().c_str() );
		}
	//	catf("%s.myid = %d", getIndent().c_str(), fv->myid );
		mlvNest --;
	}
	
	STRUCTPRM* stprm ( &mdbginfo.ctx->mem_minfo[fv->customid] );
	STRUCTDAT* stdat ( &mdbginfo.ctx->mem_finfo[stprm->subid] );
	
	addItem_prmstack( base, stdat, stprm, fv->ptr );
	return;
}
#endif

//------------------------------------------------
// [add][item] prmstack
//------------------------------------------------
void CVarinfoTree::addItem_prmstack(
	const CVarinfoTree::BaseData& base,
	STRUCTDAT* stdat,
	STRUCTPRM* stprm,
	const void* prmstack
)
{
	mlvNest ++;
	
	CString sName; sName.reserve(10);
	BaseData baseChild( nullptr, getIndent() );
	
/*
	catf( "%s.id_finfo = %d", baseChild.getIndent(), stdat->subid );
	catf( "%s.id_minfo = %d", baseChild.getIndent(), stdat->prmindex );
//*/
	
	STRUCTPRM* stprmEnd(stprm + stdat->prmmax);
	
	int i( 0 );
	while ( stprm < stprmEnd )
	 {
		const void* member = ptr_cast<const char*>(prmstack) + stprm->offset;
		bool bNamed = false;

#ifdef with_Script
		{
			auto const name = getStPrmName(stprm);
			if (name) { sName = name; bNamed = true; }
		}
#endif
		int const subid = hpimod::findStPrmIndex(stprm);
		if (subid >= 0) {
			auto const name = mdbginfo.ax->getPrmName(subid);
			if (name) {
				sName = name; bNamed = true;

				// スコープ解決を取り除く
				int const iScope = sName.find('@');
				if (iScope != std::string::npos) {
					sName = sName.substr(0, iScope);
				}
			}
		}
		if (!bNamed) sName = strf(BracketIdxL "%d" BracketIdxR, i);	// 仮の名称
		baseChild.name = sName.c_str();
		
		addItem_member( baseChild, stdat, stprm, member );
		
		// structtag => メンバではないので、数えない
		if ( stprm->mptype != MPTYPE_STRUCTTAG ) {
			i ++;
		}
		++ stprm;
	}
	
	mlvNest --;
	return;
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVarinfoTree::addItem_member(
	const CVarinfoTree::BaseData& base,
	STRUCTDAT* stdat,
	STRUCTPRM* stprm,
	const void* member_const
)
{
	void* member ( const_cast<void*>(member_const) );
	
	switch ( stprm->mptype )
	{
		case MPTYPE_STRUCTTAG:
			catf( "%s.modcls = %s", base.getIndent(),
				&mdbginfo.ctx->mem_mds[stdat->nameidx]
			);
			break;
			
		// 変数 (PVal*)
	//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			MPVarData* pVarDat ( ptr_cast<MPVarData*>(member) );
			
			if ( stprm->mptype == MPTYPE_SINGLEVAR ) {
				addItem_varScalar( base, pVarDat->pval, pVarDat->aptr );
			} else {
				addItem_var( base, pVarDat->pval );
			}
			break;
		}
		// 変数 (PVal)
		case MPTYPE_LOCALVAR:
#if clhsp
		case MPTYPE_VECTOR:
		case MPTYPE_ANY:
		case MPTYPE_FLEX:
#endif
		{
			PVal* pval ( ptr_cast<PVal*>(member) );
			addItem_var( base, pval );
			break;
		}
		// thismod (MPModInst)
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
		{
#ifdef clhsp
			MPThismod* modinst = ptr_cast<MPThismod*>(member);
			addItem_modinst( base, modinst->mv );
			break;
#else
			MPThismod* thismod = ptr_cast<MPThismod*>(member);
			PVal* pvSelf( thismod->pval );
			pvSelf->offset = thismod->aptr;
			
			HspVarProc* pHvp( mdbginfo.exinfo->HspFunc_getproc(pvSelf->flag) );
			FlexValue* fv = ptr_cast<FlexValue*>(pHvp->GetPtr(pvSelf));
			addItem_flexValue( base, fv );
			break;
#endif
		}
		// ModInst (ModInst*) : clhspのみ
#ifdef clhsp
		case MPTYPE_STRUCT:
		{
			ModInst* mv2 ( *ptr_cast<ModInst**>(member) );
			addItem_modinst( base, mv2 );
			break;
		}
#endif
		// 文字列 (char**)
	//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
		{
			char* pString ( *ptr_cast<char**>(member) );
			addItem_value( base, HSPVAR_FLAG_STR, pString );
			break;
		}
		// その他
		case MPTYPE_DNUM:
		{
			addItem_value( base, HSPVAR_FLAG_DOUBLE, ptr_cast<double*>(member) );
			break;
		}
		case MPTYPE_INUM:
		{
			addItem_value( base, HSPVAR_FLAG_INT, ptr_cast<int*>(member) );
			break;
		}
		case MPTYPE_LABEL:
		{
			addItem_value( base, HSPVAR_FLAG_LABEL, ptr_cast<label_t*>(member) );
			break;
		}
		// 他 => 無視
		default:
			catf("%s%s = ignored (mptype = %d)",
				base.getIndent(), base.getName(),
				stprm->mptype
			);
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
T msgboxf(const char* sFormat, ...)
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
void CVarinfoTree::addItem_assoc( const BaseData& base, CAssoc* src )
{
	if ( !src ) {
		catf( "%s%s = (assoc: null)", base.getIndent(), base.getName() );
		return;
	}
	
	HspVarProc* pHvp = mdbginfo.exinfo->HspFunc_seekproc("assoc_k");
	StAssocMapList* head = ((StAssocMapList::GetMapList_t)(pHvp->user))( src );
	
	// 要素なし
	if ( !head ) {
		catf( "%s%s = (assoc: empty)", base.getIndent(), base.getName() );
		return;
	}
	
	// 全キーのリスト
	catf("%s%s:", base.getIndent(), base.getName() );
	mlvNest ++;
	{
		// 列挙
		for ( StAssocMapList* list = head; list != nullptr; list = list->next ) {
			BaseData baseChild ( list->key, getIndent() );
			addItem_var( baseChild, list->pval );
		//	dbgout("%p: key = %s, pval = %p, next = %p", list, list->key, list->pval, list->next );
		}
		
		// リストの解放
		for ( StAssocMapList* list = head; list != nullptr; /* */ ) {
			StAssocMapList* next = list->next;
			mdbginfo.exinfo->HspFunc_free( list );
			list = next;
		}
		
	}
	mlvNest --;
	return;
}
#endif

#ifdef with_Vector
//------------------------------------------------
// [add][item] vector
//------------------------------------------------
void CVarinfoTree::addItem_vector( const BaseData& base, CVector* src )
{
	if ( !src ) {
		catf( "%s%s = (vector: null)", base.getIndent(), base.getName() );
		return;
	}
	
	HspVarProc* pHvp = mdbginfo.exinfo->HspFunc_seekproc("vector_k");
	int    len;
	PVal** pvals = ((GetVectorList_t)(pHvp->user))( src, &len );
	
	// 要素なし
	if ( !pvals ) {
		catf( "%s%s = (vector: empty)", base.getIndent(), base.getName() );
		return;
	}
	
	// 全キーのリスト
	catf("%s%s:", base.getIndent(), base.getName() );
	mlvNest ++;
	{
		catf( "%s.length = %d", getIndent().c_str(), len );
		
		// 列挙
		for ( int i = 0; i < len; ++ i ) {
			CString  name( strf("(%d)", i) );
			BaseData baseChild ( name.c_str(), getIndent() );
			addItem_var( baseChild, pvals[i] );
		//	dbgout("%p: idx = %d, pval = %p, next = %p", list, idx, list->pval, list->next );
		}
		
	}
	mlvNest --;
	return;
}
#endif

#ifdef with_Array
//------------------------------------------------
// [add][item] array
//------------------------------------------------
void CVarinfoTree::addItem_array( const BaseData& base, CArray* src )
{
	if ( !src ) {
		catf( "%s%s = (array: null)", base.getIndent(), base.getName() );
		return;
	}
	
	HspVarProc* pHvp = mdbginfo.exinfo->HspFunc_seekproc("array_k");
	PVal* pvInner = ((GetArray_t)(pHvp->user))( src );
	
	// 要素なし
	if ( !pvInner || pvInner->len[1] == 0 ) {
		catf( "%s%s = (array: empty)", base.getIndent(), base.getName() );
		return;
	}
	
	// 表示
	addItem_varArray( base, pvInner );
	return;
}
#endif

//------------------------------------------------
// 
//------------------------------------------------

//**********************************************************
//        下請け関数
//**********************************************************
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
void CVarinfoTree::cat( const char* src )
{
	if ( mlenLimit <= 0 ) return;
	
	cat( src, strlen(src) );
	return;
}

void CVarinfoTree::cat( const char* src, size_t len )
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
void CVarinfoTree::catf( const char* format, ... )
{
	if ( mlenLimit <= 0 ) return;
	
	va_list   arglist;
	va_start( arglist, format );
	
	CString stmp( vstrf( format, arglist ) );
	size_t   len( stmp.length() );
	
	cat( stmp.c_str(), len );
	
	va_end( arglist );
	return;
}

#ifndef clhsp
//------------------------------------------------
// デバッグ文字列を生成する
// 
// @prm pBuf : 最低でも 1024 B を必要とする
//------------------------------------------------
CString CVarinfoTree::getDbgString( vartype_t type, const void* pValue )
{
#ifndef HSPVAR_FLAG_VARIANT
	static const int HSPVAR_FLAG_VARIANT( 7 );
#endif
	
	void* ptr( const_cast<void*>(pValue) );
	
	switch ( type ) {
		case HSPVAR_FLAG_STR:     return CString( ptr_cast<char*>(ptr) );
	//	case HSPVAR_FLAG_STR:     return toStringLiteralFormat( ptr_cast<char*>(ptr) );
		case HSPVAR_FLAG_COMOBJ:  return strf("comobj (0x%08X)",  address_cast( *ptr_cast<void**>(ptr) ));
		case HSPVAR_FLAG_VARIANT: return strf("variant (0x%08X)", address_cast( *ptr_cast<void**>(ptr)) );
		case HSPVAR_FLAG_DOUBLE:  return strf( "%.16f", *ptr_cast<double*>(ptr) );
		case HSPVAR_FLAG_INT:
		{
			int const val = *ptr_cast<int*>(ptr);
#ifdef with_ModPtr
			if ( ModPtr::isValid(val) ) {
				return strf("mp#%d", ModPtr::getIdx(val))
					+ getDbgString( HSPVAR_FLAG_STRUCT, ModPtr::getValue(*ptr_cast<int*>(ptr)) );
			}
#endif
			return strf( "%-10d (0x%08X)", val, val );
		}
		case HSPVAR_FLAG_LABEL:
		{
			auto const lb = *ptr_cast<label_t*>(ptr);
			int const idx = hpimod::findOTIndex(lb);
			auto const name =
				(idx >= 0 ? g_dbginfo->ax->getLabelName(idx) : nullptr);
			return (name ? strf("*%s", name) : strf("*label (0x%08X)", address_cast(lb)));
		}
#if 0
		case HSPVAR_FLAG_STRUCT:		// 拡張表示あり
		{
#if clhsp
			ModInst* mv( *ptr_cast<ModInst**>( ptr ) );
			if ( !mv ) {
				return CString( "modinst (null)" );
				
			} else {
				STRUCTPRM* stprm( &mdbginfo.ctx->mem_minfo[mv->id_minfo]  );
				STRUCTDAT* stdat( &mdbginfo.ctx->mem_finfo[stprm->subid] );

				return strf( "modinst (0x%08X) id%d(%s) ptr(%p) size(%d)",
					address_cast( mv ),
					stprm->subid,
					address_cast( &mdbginfo.ctx->mem_mds[stdat->nameidx] ),
					mv->members,
					stdat->size
				);
			}
#else
			FlexValue* fv ( ptr_cast<FlexValue*>( ptr ) );
			if ( fv->type == FLEXVAL_TYPE_NONE ) {
				return CString( "struct (empty)" ); 
			} else {
				return strf( "struct id%d ptr(0x%08X) size(%d) type(%d)",
					fv->customid, address_cast(fv->ptr), fv->size, fv->type
				);
			}
#endif
		}
#endif
#if clhsp
	//	case HSPVAR_FLAG_VECTOR:			// 拡張表示あり
	//		return strf( "vector (0x%08X) ", address_cast( ptr_cast<Vector*>(ptr) ) );
		
		case HSPVAR_FLAG_REF:
		{
			bool       bExists ( false );
			Reference* pRef    ( ptr_cast<Reference*>( ptr ) );
			
			char sReferred[64 + 32];
			
			// 静的変数から探す
			for ( int i = 0; i < mdbginfo.ctx->hsphed->max_val; ++ i ) {
				if ( pRef->pval == &mdbginfo.ctx->mem_var[i] ) {
					strcpy_s( sReferred, mdbginfo.exinfo->HspFunc_varname(i) );
					bExists = true;
					break;
				}
			}
			
			if ( !bExists ) {
				return strf( "なにかへの参照 (%p [#%d])", pRef->pval, pRef->aptr );
				
			} else {
				if ( pRef->aptr ) {
					sprintf_s( sReferred, "%s [#%d]", sReferred, pRef->aptr );
				}
				return strf( "変数 %s への参照", sReferred );
			}
		//*/
		}
#endif
		default:
		{
			const char* vartype_name = mdbginfo.exinfo->HspFunc_getproc(type)->vartype_name;
			
		//	if ( strcmp(vartype_name, "__EXTRA" ) == 0 ) {
		//		const HspVarProc* vp = mdbginfo.exinfo->HspFunc_getproc(type);
		//		int size = vp->basesize > 0 ? vp->basesize : vp->GetSize(almighty_cast<const PDAT*>(ptr));
		//	}
#ifdef with_ExtraBasics
			// 拡張基本型
			bool bSigned = false;
			
			if ( strcmp(vartype_name, "bool") == 0 ) {
				static const char* bool_name[2] = { "false", "true" };		// 名前定数
				return bool_name[ *ptr_cast<bool*>( ptr ) ? 1 : 0 ];
				
			// char : char = signed char とする
			} else if (
				   ( strcmp(vartype_name,  "char") == 0 || strcmp(vartype_name,   "signed_char") == 0 ) && (bSigned = true)
				|| ( strcmp(vartype_name, "uchar") == 0 || strcmp(vartype_name, "unsigned_char") == 0 )
			) {
				const int val = bSigned ? *ptr_cast<signed char*>( ptr ) : *ptr_cast<unsigned char*>( ptr );
				return ( val == 0 ) ? CString("0 ('\\0')") : strf("%-3d '%c'", static_cast<int>(val), static_cast<char>(val));
				
			// short
			} else if (
				   ( strcmp(vartype_name,  "short") == 0 || strcmp(vartype_name,   "signed_short") == 0 ) && (bSigned = true)
				|| ( strcmp(vartype_name, "ushort") == 0 || strcmp(vartype_name, "unsigned_short") == 0 )
			) {
				const int val = static_cast<int>( bSigned ? *ptr_cast<signed short*>( ptr ) : *ptr_cast<unsigned short*>( ptr ) );
				return strf( "%-6d (0x%04X)", val, static_cast<short>(val) );
				
			// unsigned int
			} else if ( strcmp(vartype_name, "uint") == 0 || strcmp(vartype_name, "unsigned_int") == 0 ) {
				const auto val = *ptr_cast<unsigned int*>( ptr );
				return strf( "%-10d (0x%08X)", val, val );
				
			// long
			} else if (
				   ( strcmp(vartype_name,  "long") == 0 || strcmp(vartype_name,   "signed_long") == 0 ) && (bSigned = true)
				|| ( strcmp(vartype_name, "ulong") == 0 || strcmp(vartype_name, "unsigned_long") == 0 ) 
			) {
				const auto signed_val = *ptr_cast<long long*>( ptr );
				return strf( "%d (0x%16X)", bSigned ? signed_val : *ptr_cast<unsigned long long*>(ptr), signed_val );
			/*
			} else if ( strcmp(vartype_name,  "long") == 0 || strcmp(vartype_name,   "signed_long") == 0 ) {
				const auto val = *ptr_cast<long long*>( ptr );
				return strf( "%d (0x%16X)", val, val );
				
			} else if ( strcmp(vartype_name, "ulong") == 0 || strcmp(vartype_name, "unsigned_long") == 0 ) {
				const auto val = *ptr_cast<unsigned long long*>( ptr );
				return strf( "%d (0x%16X)", val, val );
			//*/
			// おまけ
			} else if ( strcmp(vartype_name, "tribyte") == 0 ) {
				const char* bytes = ptr_cast<char*>( ptr );
				const int   val   = bytes[0] << 16 | bytes[1] << 8 | bytes[2];		// 再構成
				return strf( "%-8d (0x%06X)", val, val );
			}
#endif
#ifdef with_Modcmd
			// "modcmd_k" 型
			if ( strcmp(vartype_name, "modcmd_k") == 0 ) {
				int const modcmd = *ptr_cast<int*>(ptr);
				return strf("modcmd(%s)",
					(modcmd == 0xFFFFFFFF) ? "" : &mdbginfo.ctx->mem_mds[mdbginfo.ctx->mem_finfo[modcmd].nameidx]
				);
			}
#endif
			return strf( "Unknown<%s>: 0x%08X", vartype_name, address_cast( ptr ) );
		}
	}
}

//------------------------------------------------
// 文字列を文字列リテラルの形式に変換する
//------------------------------------------------
CString CVarinfoTree::toStringLiteralFormat( const char* src )
{
	char* buf = mdbginfo.exinfo->HspFunc_malloc( strlen(src) * 2 );
	uint iWrote( 0 );
	
	buf[iWrote ++] = '\"';
	
	for ( int i = 0; i < 0x400; ++ i ) {
		char c = src[i];
		
		// エスケープ・シーケンスを解決する
		if ( c == '\0' ) {
			break;
			
		} else if ( c == '\\' || c == '\"' ) {
			buf[iWrote ++] = '\\';
			buf[iWrote ++] = c;
			
		} else if ( c == '\t' ) {
			buf[iWrote ++] = '\\';
			buf[iWrote ++] = 't';
			
		} else if ( c == '\r' || c == '\n' ) {
			buf[iWrote ++] = '\\';
			
			if ( c == '\r' && src[i + 1] == '\n' ) {	// CR + LF
				buf[iWrote ++] = 'n';
				i ++;
			} else {
				buf[iWrote ++] = 'r';
			}
			
		} else {
			buf[iWrote ++] = c;
		}
	}
	
	buf[iWrote ++] = '\"';
	buf[iWrote ++] = '\0';
	
	CString sResult ( buf );
	
	mdbginfo.exinfo->HspFunc_free( buf );
	
	return sResult;
}

#endif

//------------------------------------------------
// mptype の文字列を得る
//------------------------------------------------
static const char* getMPTypeString( int mptype )
{
	switch ( mptype ) {
		case MPTYPE_NONE:    return "(unknown)";
		case MPTYPE_DNUM:    return "double";
		case MPTYPE_INUM:    return "int";
		
		case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING: return "str";
		case MPTYPE_VAR:
		case MPTYPE_PVARPTR:				// #dllfunc
		case MPTYPE_SINGLEVAR:   return "var";
		case MPTYPE_ARRAYVAR:    return "array";
		case MPTYPE_LOCALVAR:    return "local";
		case MPTYPE_MODULEVAR:   return "modvar";
		case MPTYPE_IMODULEVAR:  return "modinit";
		case MPTYPE_TMODULEVAR:  return "modterm";
		
#ifdef clhsp
		case MPTYPE_LABEL:  return "label";
		case MPTYPE_STRUCT: return "struct";
	//	case MPTYPE_REF:    return "ref";
		case MPTYPE_VECTOR: return "vector";
		case MPTYPE_ANY:    return "any";
		case MPTYPE_FLEX:   return "...";		// flex
#endif
		
#if 0
		case MPTYPE_IOBJECTVAR:  return "comobj";
	//	case MPTYPE_LOCALWSTR:   return "";
	//	case MPTYPE_FLEXSPTR:    return "";
	//	case MPTYPE_FLEXWPTR:    return "";
		case MPTYPE_FLOAT:       return "float";
		case MPTYPE_PPVAL:       return "pval";
		case MPTYPE_PBMSCR:      return "bmscr";
		case MPTYPE_PTR_REFSTR:  return "prefstr";
		case MPTYPE_PTR_EXINFO:  return "pexinfo";
		case MPTYPE_PTR_DPMINFO: return "pdpminfo";
		case MPTYPE_NULLPTR:     return "nullptr";
		
		case MPTYPE_STRUCTTAG:   return "(structtag)";
#endif
		default: return "unknown";
	}
}

CString makeArrayIndexString(uint dim, uint const indexes[])
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

