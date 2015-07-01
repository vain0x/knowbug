// 変数データテキスト生成クラス

#include "CVarinfoText.h"
#include "CVarinfoTree.h"
#include "module/mod_cast.h"

#ifdef clhsp
# include "hsp3/mod_vector.h"
#endif

#include "SysvarData.h"

//##############################################################################
//                定義部 : CVarinfoText
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CVarinfoText::CVarinfoText( DebugInfo& dbginfo, int lenLimit )
	: mdbginfo ( dbginfo )
	, mpBuf    ( new CString )
	, mpVar    ( NULL )
	, mpName   ( NULL )
	, mlenLimit( lenLimit )
{
	mpBuf->reserve( std::min(0x400, lenLimit) );		// 0x400 はテキトー
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CVarinfoText::~CVarinfoText()
{
	delete mpBuf; mpBuf = NULL;
	return;
}

//------------------------------------------------
// 変数データから生成
//------------------------------------------------
void CVarinfoText::addVar( PVal *pval, const char *name )
{
	mpVar  = pval;
	mpName = name;
	make();
	return;
}

//------------------------------------------------
// 生成
//------------------------------------------------
void CVarinfoText::make( void )
{
	PVal*& pval = mpVar;
	
	HspVarProc* pHvp = mdbginfo.exinfo->HspFunc_getproc( pval->flag );
	int bufsize; 
	void* pMemBlock = pHvp->GetBlockSize( pval, ptr_cast<PDAT*>(pval->pt), ptr_cast<int*>(&bufsize) );
	
	// 変数に関する情報
	catf( "変数名：%s", mpName );
	catf( "変数型：%s " BracketIdxL "%d, %d, %d" BracketIdxR,
		pHvp->vartype_name,
		PValLength( pHvp, pval, 1 ),
		PValLength( pHvp, pval, 2 ),
		PValLength( pHvp, pval, 3 )
	);
	catf( "モード：%s", getModeString( pval->mode ) );
	catf( "アドレス：0x%08X, 0x%08X", address_cast(pval->pt), address_cast(pval->master) );
	catf( "サイズ：using %d in %d [byte]", pval->size, bufsize );
	
	cat_crlf();
	
	// 変数の内容に関する情報
	{
		CVarinfoTree *varinf( new CVarinfoTree( mdbginfo, mlenLimit ) );
		
		varinf->addVar( pval, mpName );
		
		const CString& sTree( varinf->getString() );
		size_t len( sTree.size() );		// mlenLimit は越えてない
		cat( sTree.c_str() );
		
		mlenLimit -= len;
		
		delete varinf;
	}
	
	// メモリダンプ
	dump( pMemBlock, static_cast<size_t>(bufsize) );
	
	return;
}

/*
//------------------------------------------------
// 変数をメモリダンプする
//------------------------------------------------
void CVarinfoText::dumpVar( PVal *pval )
{
	size_t size;
	
	HspVarCoreReset( pval );
	HspVarProc *pHvp ( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	void *ptr ( pHvp->GetPtr( pval ) );
	void *mem (  );
	
	dump( mem, size );
	
	return;
}
//*/

//------------------------------------------------
// メモリダンプを追加する
//------------------------------------------------
void CVarinfoText::dump( void *mem, size_t bufsize )
{
	static const size_t stc_maxsize( 0x10000 );
	size_t size( bufsize );
	
	if ( size > stc_maxsize ) {
		catf( "全%d[byte]の内、%d[byte]のみをダンプします。", bufsize, stc_maxsize );
		size = stc_maxsize;
	}
	
	char tline[1024];
	size_t iWrote;
	uint idx ( 0 );
	
	cat("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	cat("----------------------------------------------------");
	
	while ( idx < size ) {
		iWrote = sprintf_s( tline, "%04X", idx );
		
		for ( uint i = 0; (i < 0x10 && idx < size); ++ i, ++ idx ) {
			iWrote += sprintf_s(
				&tline[iWrote], 1024 - iWrote,
				" %02X", static_cast<unsigned char>( ptr_cast<char *>(mem)[idx] )
			);
		}
		
		cat( tline );
	}
	
	return;
}

//------------------------------------------------
// システム変数データから生成
//------------------------------------------------
void CVarinfoText::addSysvar( const char *name )
{
	int idx ( -1 );
	for ( int i = 0; i < SysvarCount; ++ i ) {
		if ( strcmp( name, SysvarData[i].name ) == 0 ) {
			idx = i;
			break;
		}
	}
	if ( idx < 0 ) return;
	
	mpVar  = NULL;
	mpName = name;
	
	vartype_t   type ( SysvarData[idx].type );
	HspVarProc *pHvp ( mdbginfo.exinfo->HspFunc_getproc( type ) );
	
	catf( "変数名：%s\t(システム変数)", mpName );
	catf( "変数型：%s", pHvp->vartype_name );
	cat_crlf();
	
	void  *pDumped   ( NULL );
	size_t sizeToDump( 0 );

	{
		CVarinfoTree* varinf = new CVarinfoTree( mdbginfo, mlenLimit );
		
		varinf->addSysvar( idx, name, &pDumped, &sizeToDump );
		
		const CString& sTree = varinf->getString();
		size_t len = sTree.length();
		cat( sTree.c_str() );				// 内容を連結する
		
		mlenLimit -= len;
		delete varinf;
	}
	
	if ( pDumped != NULL ) {
		cat_crlf();
		dump( pDumped, sizeToDump );
	}
	
	return;
}

#if with_WrapCall
//------------------------------------------------
// 引数タイプの文字列を得る
//------------------------------------------------
static const char *getMptypeString( STRUCTPRM *pStPrm )
{
	switch ( pStPrm->mptype ) {
	//	case MPTYPE_STRUCTTAG:   return "structtag";
		case MPTYPE_LABEL:       return "label";
		case MPTYPE_DNUM:        return "double";
		case MPTYPE_INUM:        return "int";
		case MPTYPE_LOCALSTRING: return "str";
		case MPTYPE_STRUCT:      return "modcls";
		case MPTYPE_MODULEVAR:   return "thismod";
		case MPTYPE_IMODULEVAR:  return "thismod(new)";
		case MPTYPE_TMODULEVAR:  return "thismod(delete)";
		case MPTYPE_SINGLEVAR:   return "var";
		case MPTYPE_ARRAYVAR:    return "array";
		case MPTYPE_LOCALVAR:    return "local";
#ifdef clhsp
		case MPTYPE_ANY:         return "any";
		case MPTYPE_VECTOR:      return "vector";
		case MPTYPE_FLEX:        return "...";
		default:
			// モジュールクラス
			if ( pStPrm->mptype >= MPTYPE_MODCLS_BIAS ) {
				int idxFinfo( pStPrm->mptype - MPTYPE_MODCLS_BIAS );
				return &mdbginfo.ctx->mem_mds[mdbginfo.ctx->mem_finfo[idxFinfo].nameidx];
				
				ModInst *mv( code_get_modinst() );
				*ptr_cast<ModInst **>(out) = mv;
				
				if ( get_stprm(mv)->subid != idxFinfo ) {
					throw runerr HSPERR_TYPE_MISMATCH;
				}
			}
			break;
#endif
	}
	return "";
}

//------------------------------------------------
// 呼び出しデータから生成
//------------------------------------------------
void CVarinfoText::addCall( STRUCTDAT* pStDat, void *prmstk, int sublev, const char *name )
{
	catf( "関数名：%s", name );
	
	// シグネチャ
	{
		CString sPrm = "仮引数：(";
		STRUCTPRM *pStPrm = &mdbginfo.ctx->mem_minfo[pStDat->prmindex];

		if ( pStDat->prmmax == 0 ) {
			sPrm += "void";
		} else {
			for ( int i = 0; i < pStDat->prmmax; ++ i ) {
				if ( i !=0 ) sPrm += ", ";
				sPrm += getMptypeString( pStPrm + i );
			}
		}
		
		sPrm += ")";
		cat( sPrm.c_str() );
	}
	
	if ( prmstk == NULL ) {
		cat( "(unknown arguments)" );
	} else {
		// 変数の内容に関する情報
		{
			CVarinfoTree *varinf( new CVarinfoTree( mdbginfo, mlenLimit ) );
			
			varinf->addCall( pStDat, prmstk, name );
			
			const CString& sTree( varinf->getString() );
			size_t len( sTree.size() );		// mlenLimit は越えてない
			cat( sTree.c_str() );
			
			mlenLimit -= len;
			
			delete varinf;
		}
		
		// メモリダンプ
		dump( prmstk, pStDat->size );
	}
	
	return;
}
#endif

//**********************************************************
//        下請け関数
//**********************************************************
//------------------------------------------------
// 改行を連結する
//------------------------------------------------
void CVarinfoText::cat_crlf( void )
{
	if ( mlenLimit < 2 ) return;
	
	mpBuf->append( "\r\n" );
	mlenLimit -= 2;
	return;
}

//------------------------------------------------
// 文字列を連結する
//------------------------------------------------
void CVarinfoText::cat( const char *string )
{
	if ( mlenLimit <= 0 ) return;
	
	size_t len( strlen( string ) + 2 );		// 2 は crlf の分
	
	if ( static_cast<int>(len) > mlenLimit ) {
		mpBuf->append( string, mlenLimit );
		mpBuf->append( "(長すぎたので省略しました。)" );
		mlenLimit = 2;
	} else {
		mpBuf->append( string );
		mlenLimit -= len;
	}
	
	cat_crlf();
	return;
}

//------------------------------------------------
// 書式付き文字列を連結する
//------------------------------------------------
void CVarinfoText::catf( const char *format, ... )
{
	va_list   arglist;
	va_start( arglist, format );
	
	cat( vstrf( format, arglist ).c_str() );
	
	va_end( arglist );
	return;
}
