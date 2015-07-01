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
	PVal *& pval = mpVar;
	
	HspVarProc *pHvp ( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	
	// 変数に関する情報
	catf( "変数名：%s", mpName );
	catf( "変数型：%s", pHvp->vartype_name );
	catf( "配列 " BracketIdxL "%d, %d, %d" BracketIdxR,
		PValLength( pHvp, pval, 1 ),
		PValLength( pHvp, pval, 2 ),
		PValLength( pHvp, pval, 3 )
	);
	catf( "モード：%s", getModeString( pval->mode ) );
	catf( "アドレス：0x%08X", address_cast(pval->pt) );
	catf( "マスター：0x%08X", address_cast(pval->master) );
	catf( "使用サイズ：%d (bytes)",   pval->size );
	
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
	dumpVar( pval );
	
	return;
}

//------------------------------------------------
// 変数をメモリダンプする
//------------------------------------------------
void CVarinfoText::dumpVar( PVal *pval )
{
	size_t size;
	
	HspVarCoreReset( pval );
	HspVarProc *pHvp ( mdbginfo.exinfo->HspFunc_getproc( pval->flag ) );
	void *ptr ( pHvp->GetPtr( pval ) );
	void *mem ( pHvp->GetBlockSize( pval, ptr_cast<PDAT *>(ptr), ptr_cast<int *>(&size) ) );
	
	dump( mem, size );
	
	return;
}

//------------------------------------------------
// メモリダンプを追加する
//------------------------------------------------
void CVarinfoText::dump( void *mem, size_t bufsize )
{
	static const size_t stc_maxsize( 0x10000 );
	size_t size( bufsize );
	
	if ( size > stc_maxsize ) {
		catf( "全%dバイトの内、%dバイトのみをダンプします。", bufsize, stc_maxsize );
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
	
	cat_crlf();
	catf( "バッファサイズ：%d (bytes)", bufsize );
	
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
	catf( "変数型：%s" , pHvp->vartype_name );
	cat_crlf();
	
	void  *pDumped   ( NULL );
	size_t sizeToDump( 0 );
	
	switch ( idx ) {
		// 整数値
		case SysvarId_Stat:
		case SysvarId_IParam:
		case SysvarId_WParam:
		case SysvarId_LParam:
		case SysvarId_StrSize:
		case SysvarId_Looplev:
		case SysvarId_Sublev:
		case SysvarId_Err:
	//	case SysvarId_MouseX:
	//	case SysvarId_MouseY:
	//	case SysvarId_MouseW:
		{
			int *p;
			switch ( idx ) {
				case SysvarId_Stat:    p = &mdbginfo.ctx->stat;    break;
				case SysvarId_IParam:  p = &mdbginfo.ctx->iparam;  break;
				case SysvarId_WParam:  p = &mdbginfo.ctx->wparam;  break;
				case SysvarId_LParam:  p = &mdbginfo.ctx->lparam;  break;
				case SysvarId_StrSize: p = &mdbginfo.ctx->strsize; break;
				case SysvarId_Looplev: p = &mdbginfo.ctx->looplev; break;
				case SysvarId_Sublev:  p = &mdbginfo.ctx->sublev;  break;
				case SysvarId_Err:     p = ptr_cast<int *>( &mdbginfo.ctx->err ); break;
			}
			catf( "%s = %-10d (0x%08X)", name, *p, *p );
			pDumped    = p;
			sizeToDump = sizeof(int);
			break;
		}
		// refstr
		case SysvarId_Refstr:
		{
			char *& str = mdbginfo.ctx->refstr;
			catf( "%s = %s", name, str );
			pDumped    = str;
			sizeToDump = HSPCTX_REFSTR_MAX;
			break;
		}
		// refdval
		case SysvarId_Refdval:
		{
			double& dval = mdbginfo.ctx->refdval;
			catf( "%s = %.16f", name, dval );
			pDumped    = &dval;
			sizeToDump = sizeof(dval);
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
					int& cnt = ptr_cast<LOOPDAT *>( &mdbginfo.ctx->mem_loop )[lvLoop].cnt;
#endif
					catf( "   #%d = %d", lvLoop, cnt );
				}
			}
			break;
		}
		// thismod
		case SysvarId_Thismod:
		{
			if ( mdbginfo.ctx->prmstack != NULL ) {
				MPThismod *thismod = ptr_cast<MPThismod *>( mdbginfo.ctx->prmstack );
				
				if ( thismod->magic == MODVAR_MAGICCODE ) {
					CVarinfoTree varinf( mdbginfo );
#ifdef clhsp
					varinf.addModInst( thismod->mv, "thismod" );
#else
					PVal *pval( thismod->pval );
					pval->offset = thismod->aptr;
					
					HspVarProc *pHvp( mdbginfo.exinfo->HspFunc_getproc(pval->flag) );
					varinf.addFlexValue( ptr_cast<FlexValue *>(pHvp->GetPtr(pval)), "thismod" );
				//	varinf.addVar( thismod->pval, "thismod" );
#endif
					cat( varinf.getString().c_str() );
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
	
	if ( pDumped != NULL ) {
		cat_crlf();
		dump( pDumped, sizeToDump );
	}
	
	return;
}

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
