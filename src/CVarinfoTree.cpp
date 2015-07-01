// 変数データのツリー形式文字列

#include "CVarinfoTree.h"
#include "module/mod_cast.h"

#ifdef clhsp
# include "hsp3/mod_vector.h"
#endif

static const char *getMPTypeString( int mptype );

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
	mpBuf->reserve( std::min(0x400, mlenLimit + 1) );
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CVarinfoTree::~CVarinfoTree()
{
	delete mpBuf; mpBuf = NULL;
	mlvNest = 0;
	return;
}

//------------------------------------------------
// [add] 変数
//------------------------------------------------
void CVarinfoTree::addVar( PVal *pval, const char *name )
{
	BaseData base ( name, getIndent() );
	
	addItem_var( base, pval );
	return;
}

#if 0
//------------------------------------------------
// [add] 単体変数
//------------------------------------------------
void CVarinfoTree::addVarScalar( PVal *pval, const char *name )
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
void CVarinfoTree::addModInst( ModInst *mv, const char *name )
{
	BaseData base ( name, getIndent() );
	addItem_modinst( base, mv );
	return;
}

#else

//------------------------------------------------
// [add] flexValue
//------------------------------------------------
void CVarinfoTree::addFlexValue( FlexValue *fv, const char *name )
{
	BaseData base ( name, getIndent() );
	addItem_flexValue( base, fv );
	return;
}
#endif

//------------------------------------------------
// [add][item] 値
//------------------------------------------------
void CVarinfoTree::addItem_value( const BaseData& base, vartype_t type, void *ptr )
{
	if ( type == HSPVAR_FLAG_STRUCT ) {
#ifdef clhsp
		addItem_modinst( base, *ptr_cast<ModInst **>(ptr) );
#else
		addItem_flexValue( base, ptr_cast<FlexValue *>(ptr) );
#endif
//	} else if ( type == HSPVAR_FLAG_STR ) {
//		addItem_string( base, ptr_cast<char *>(ptr) );
		
	} else {
#ifdef clhsp
		char *p = mdbginfo.debug->dbg_toString( type, ptr );
		
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
void CVarinfoTree::addItem_var( const BaseData& base, PVal *pval )
{
	HspVarProc *pHvp( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	int       length( PValLength( pHvp, pval, 1 ) );
	
#if clhsp
	// 空
	if ( length == 0 ) {
		catf( "%s%s (empty)", base.getIndent(), base.getName() );
		
	// 単体
	} else if ( ( length == 1 && PValLength(pHvp, pval, 2) == 0 )
		&& pval->flag != HSPVAR_FLAG_VECTOR
	) {
		pval->offset = 0;
		addItem_varScalar( base, pval );
		
	// 配列
	} else {
#endif
		catf( "%s%s:", base.getIndent(), base.getName() );
		
		mlvNest ++;
		
		if ( mlvNest > 300 ) {
			catf( "%s[Error] (too many nesting)", getIndent() );
		} else
#ifdef clhsp
		if ( pval->flag == HSPVAR_FLAG_VECTOR ) {
			addItem_vector( base, ptr_cast<Vector *>( pval->pt ), length );
		} else
#endif
		{
			addItem_varArray( base, pval );
		}
		
		mlvNest --;
#if clhsp
	}
#endif
	
	return;
}

//------------------------------------------------
// [add][item] 単体変数
//------------------------------------------------
void CVarinfoTree::addItem_varScalar( const CVarinfoTree::BaseData& base, PVal *pval )
{
	HspVarProc *pHvp( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	
	addItem_value( base, pval->flag, pHvp->GetPtr(pval) );
	return;
}

//------------------------------------------------
// [add][item] 配列変数 (array)
// 
// @ vector 型は来ない。
// @prm base : 親要素(配列変数)のある深さ
//------------------------------------------------
void CVarinfoTree::addItem_varArray( const CVarinfoTree::BaseData& base, PVal *pval )
{
	BaseData baseChild( NULL, getIndent() );
	
	HspVarProc *pHvp( mdbginfo.exinfo->HspFunc_getproc(pval->flag) );
	
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
			baseChild.getIndent(),
			strf( stc_fmt_elemname[maxdim], length[0], length[1], length[2] ).c_str(),
			cntElem
		);
	}

	// 各要素を追加する
	for ( uint i = 0; i < cntElem; ++ i ) {
		
		// aptr を分解して添字を求める
		APTR aptr ( i );
		APTR idx[ArrayDimMax] = { 0 };
		for ( uint idxDim = 0; idxDim < maxdim; ++ idxDim ) {
			idx[idxDim] = aptr % length[idxDim];
			aptr        = aptr / length[idxDim];
		}
		
		CString sName( strf(stc_fmt_elemname[maxdim], idx[0], idx[1], idx[2]) );
		baseChild.name = sName.c_str();
		
		// 要素の値を追加 ( 便宜上、単体変数として追加する )
		pval->offset = i;
		addItem_varScalar( baseChild, pval );
	}
	
	return;
}

//------------------------------------------------
// [add][item] 変数 (vartype: str)
//------------------------------------------------
void CVarinfoTree::addItem_varStr( const BaseData& base, PVal *pval, bool bScalar )
{
	if ( bScalar ) {
		catf( "%s", ptr_cast<char *>( pval->pt ) );
		
	} else {
		addItem_varArray( base, pval );
	}
	return;
}

#ifdef clhsp
//------------------------------------------------
// [add][item] vector
//------------------------------------------------
void CVarinfoTree::addItem_vector( const BaseData& base, Vector *vec, int length )
{
//	CString sName;
//	BaseData baseChild( NULL, getIndent() );
	
	for ( int i = 0; i < length; ++ i ) {
	//*
		addItem_vecelem( base, vec->list->at(i), i );
	/*/
		PVal *pvElem = vec->list->at( i )->pval;
		
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
void CVarinfoTree::addItem_vecelem( const BaseData& base, VecElem *vecelem, int idx )
{
	PVal *pval ( vecelem->pval );
	APTR  aptr ( vecelem->aptr );
	
	CString sName(
		strf( "%s%s",
			strf("[%d]", idx).c_str(),
			( (aptr < 0) ? "*" : "&" )
		)
	);
	
	mlvNest ++;
	
	const char *pName  ( sName.c_str() );
	BaseData  baseChild( pName, getIndent() );
	const char *pIndent( baseChild.getIndent() );
	
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
void CVarinfoTree::addItem_modinst( const BaseData& base, ModInst *mv )
{
	if ( mv == NULL ) {
		catf( "%s%s = (nullmod)", base.getIndent(), base.getName() );
		return;
	}
	
	catf( "%s%s:", base.getIndent(), base.getName() );
	
	STRUCTPRM *pStPrm ( &mdbginfo.ctx->mem_minfo[mv->id_minfo] );
	STRUCTDAT *pStDat ( &mdbginfo.ctx->mem_finfo[pStPrm->subid] );
	void      *prmstack ( mv->members );
	
	addItem_prmstack( base, pStDat, pStPrm, prmstack );
	
	return;
}

#else

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVarinfoTree::addItem_flexValue( const BaseData& base, FlexValue *fv )
{
	if ( fv == NULL ) {
		catf( "%s[error] (addItem_flexValue) fv = NULL", base.getIndent() );
		return;
	}
	
	if ( fv->ptr == NULL || fv->type == FLEXVAL_TYPE_NONE ) {
		catf( "%s%s = (empty struct)", base.getIndent(), base.getName() );
		return;
	}
	
	catf( "%s%s:", base.getIndent(), base.getName() );
	
	if ( fv->type == FLEXVAL_TYPE_CLONE ) {
		mlvNest ++;
		catf( "%s@clone", getIndent().c_str() );
		mlvNest --;
	}
	
	STRUCTPRM *pStPrm ( &mdbginfo.ctx->mem_minfo[fv->customid] );
	STRUCTDAT *pStDat ( &mdbginfo.ctx->mem_finfo[pStPrm->subid] );
	
	addItem_prmstack( base, pStDat, pStPrm, fv->ptr );
	return;
}
#endif

//------------------------------------------------
// [add][item] prmstack
//------------------------------------------------
void CVarinfoTree::addItem_prmstack(
	const CVarinfoTree::BaseData& base,
	STRUCTDAT *pStDat,
	STRUCTPRM *pStPrm,
	const void *prmstack
)
{
	mlvNest ++;
	
	CString sName; sName.reserve( 5 );
	BaseData baseChild( NULL, getIndent() );
	
/*
	catf( "%s.id_finfo = %d", baseChild.getIndent(), pStDat->subid );
	catf( "%s.id_minfo = %d", baseChild.getIndent(), pStDat->prmindex );
//*/
	
	STRUCTPRM *pStPrmEnd( pStPrm + pStDat->prmmax );
	
	int i( 0 );
	while ( pStPrm < pStPrmEnd )
	 {
		const void *member ( ptr_cast<const char *>(prmstack) + pStPrm->offset );
		
		sName = strf( BracketIdxL "%d" BracketIdxR, i );	// 仮の名称
		baseChild.name = sName.c_str();
		
		addItem_member( baseChild, pStDat, pStPrm, member );
		
		// structtag => メンバではないので、数えない
		if ( pStPrm->mptype != MPTYPE_STRUCTTAG ) {
			i ++;
		}
		++ pStPrm;
	}
	
	mlvNest --;
	return;
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVarinfoTree::addItem_member(
	const CVarinfoTree::BaseData& base,
	STRUCTDAT *pStDat,
	STRUCTPRM *pStPrm,
	const void *member_const
)
{
	void *member ( const_cast<void *>(member_const) );
	
	switch ( pStPrm->mptype )
	{
		case MPTYPE_STRUCTTAG:
			catf( "%s.modcls = %s", base.getIndent(),
				&mdbginfo.ctx->mem_mds[pStDat->nameidx]
			);
			break;
			
		// 変数 (PVal *)
	//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR:
		{
			MPVarData *pVarDat ( ptr_cast<MPVarData *>(member) );
			
			pVarDat->pval->offset = pVarDat->aptr;
			
			addItem_var( base, pVarDat->pval );
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
			PVal *pval ( ptr_cast<PVal *>(member) );
			addItem_var( base, pval );
			break;
		}
		// thismod (MPModInst)
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
		{
#ifdef clhsp
			MPModInst *modinst ( ptr_cast<MPModInst *>(member) );
			addItem_modinst( base, modinst->mv );
			break;
#else
			MPModVarData *modvar ( ptr_cast<MPModVarData *>(member) );
			PVal *pvSelf ( modvar->pval );
			pvSelf->offset = modvar->aptr;
			addItem_varScalar( base, pvSelf );
			break;
#endif
		}
		// ModInst (ModInst *) : clhspのみ
#ifdef clhsp
		case MPTYPE_STRUCT:
		{
			ModInst *mv2 ( *ptr_cast<ModInst **>(member) );
			addItem_modinst( base, mv2 );
			break;
		}
#endif
		// 文字列 (char **)
	//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
		{
			char *pString ( *ptr_cast<char **>(member) );
			addItem_value( base, HSPVAR_FLAG_STR, pString );
			break;
		}
		// その他
		case MPTYPE_DNUM:
		{
			addItem_value( base, HSPVAR_FLAG_DOUBLE, ptr_cast<double *>(member) );
			break;
		}
		case MPTYPE_INUM:
		{
			addItem_value( base, HSPVAR_FLAG_INT, ptr_cast<int *>(member) );
			break;
		}
		case MPTYPE_LABEL:
		{
			addItem_value( base, HSPVAR_FLAG_LABEL, ptr_cast<label_t *>(member) );
			break;
		}
		// 他 => 無視
		default:
			catf("%s%s = ignored (mptype = %d)",
				base.getIndent(), base.getName(),
				pStPrm->mptype
			);
			break;
	}
	return;
}

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
	if ( mlenLimit < 2 ) return;
	
	mpBuf->append( "\r\n" );
	mlenLimit -= 2;
	return;
}

//------------------------------------------------
// 書式付き文字列を連結する
//------------------------------------------------
void CVarinfoTree::catf( const char *format, ... )
{
	if ( mlenLimit <= 0 ) return;
	
	va_list   arglist;
	va_start( arglist, format );
	
	CString stmp( vstrf( format, arglist ) );
	size_t   len( stmp.length() + 2 );		// 2 は crlf の分
	
	if ( static_cast<int>(len) > mlenLimit ) {		// 限界なら
		mpBuf->append( stmp.c_str(), mlenLimit );
		mpBuf->append( "(長すぎたので省略しました。)" );
		mlenLimit  = 2;
	} else {
		mpBuf->append( stmp );
		mlenLimit -= len;
	}
	cat_crlf();
	
	va_end( arglist );
	return;
}

#ifndef clhsp
//------------------------------------------------
// デバッグ文字列を生成する
// 
// @prm pBuf : 最低でも 1024 B を必要とする
//------------------------------------------------
CString CVarinfoTree::getDbgString( vartype_t type, const void *pValue )
{
#ifndef HSPVAR_FLAG_VARIANT
	static const int HSPVAR_FLAG_VARIANT( 7 );
#endif
	
	void *ptr( const_cast<void *>(pValue) );
	
	switch ( type ) {
		case HSPVAR_FLAG_STR:     return CString( ptr_cast<char *>(ptr) );
	//	case HSPVAR_FLAG_STR:     return toStringLiteralFormat( ptr_cast<char *>(ptr) );
		case HSPVAR_FLAG_LABEL:   return strf("*label (0x%08X)",  address_cast( *ptr_cast<label_t *>(ptr)) );
		case HSPVAR_FLAG_COMOBJ:  return strf("comobj (0x%08X)",  address_cast( *ptr_cast<void **>(ptr) ));
		case HSPVAR_FLAG_VARIANT: return strf("variant (0x%08X)", address_cast( *ptr_cast<void **>(ptr)) );
		case HSPVAR_FLAG_DOUBLE:  return strf( "%.16f", *ptr_cast<double *>(ptr) );
		case HSPVAR_FLAG_INT:
		{
			int val ( *ptr_cast<int *>(ptr) );
			return strf( "%-10d (0x%08X)", val, val );
		}
		
		case HSPVAR_FLAG_STRUCT:
		{
#if clhsp
			ModInst *mv( *ptr_cast<ModInst **>( ptr ) );
			if ( mv == NULL ) {
				return CString( "modinst (null)" );
				
			} else {
				STRUCTPRM *pStPrm( &mdbginfo.ctx->mem_minfo[mv->id_minfo]  );
				STRUCTDAT *pStDat( &mdbginfo.ctx->mem_finfo[pStPrm->subid] );

				return strf( "modinst (0x%08X) id%d(%s) ptr(%p) size(%d)",
					address_cast( mv ),
					pStPrm->subid,
					address_cast( &mdbginfo.ctx->mem_mds[pStDat->nameidx] ),
					mv->members,
					pStDat->size
				);
			}
#else
			FlexValue *fv ( ptr_cast<FlexValue *>( ptr ) );
			if ( fv->type == FLEXVAL_TYPE_NONE ) {
				return CString( "struct (empty)" ); 
			} else {
				return strf( "struct id%d ptr(0x%08X) size(%d) type(%d)",
					fv->customid, address_cast(fv->ptr), fv->size, fv->type
				); 
			}
#endif
		}
#if clhsp
		case HSPVAR_FLAG_VECTOR:
			return strf( "vector (0x%08X) ", address_cast( ptr_cast<Vector *>(ptr) ) );
		
		case HSPVAR_FLAG_REF:
		{
			bool       bExists ( false );
			Reference *pRef    ( ptr_cast<Reference *>( ptr ) );
			
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
			return strf( "Unknown<%s>: ptr(0x%08X)",
				mdbginfo.exinfo->HspFunc_getproc(type)->vartype_name,
				address_cast( ptr )
			);
	}
}

//------------------------------------------------
// 文字列を文字列リテラルの形式に変換する
//------------------------------------------------
CString CVarinfoTree::toStringLiteralFormat( const char *src )
{
	char *buf = mdbginfo.exinfo->HspFunc_malloc( strlen(src) * 2 );
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
static const char *getMPTypeString( int mptype )
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

//**********************************************************
//    変数の初期化
//**********************************************************
const char *stc_fmt_elemname[5] = {
	""         "\0%d%d%d",
	BracketIdxL "%d"         BracketIdxR "\0%d%d",
	BracketIdxL "%d, %d"     BracketIdxR "\0%d",
	BracketIdxL "%d, %d, %d" BracketIdxR,
	BracketIdxL "%d, %d, %d, ?"
};
