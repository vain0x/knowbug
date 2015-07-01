// 変数データテキスト生成クラス

#include <numeric>	// for accumulate
#include <algorithm>

#include "module/ptr_cast.h"

#include "main.h"
#include "SysvarData.h"
#include "CVarinfoText.h"
#include "CVarinfoTree.h"

#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
using namespace WrapCall;
#endif

//------------------------------------------------
// 構築
//------------------------------------------------
CVarinfoText::CVarinfoText(int lenLimit)
	: mBuf(lenLimit)
{
	mBuf.reserve(0x400);		// 0x400 はテキトー
	return;
}

//------------------------------------------------
// 変数データから生成
//------------------------------------------------
void CVarinfoText::addVar( PVal* pval, char const* name )
{
	auto const hvp = hpimod::getHvp(pval->flag);
	int bufsize;
	void const* const pMemBlock =
		hvp->GetBlockSize(pval, ptr_cast<PDAT*>(pval->pt), ptr_cast<int*>(&bufsize));

	// 変数に関する情報
	mBuf.catln(strf("変数名：%s", name));
	mBuf.catln(strf("変数型：%s", getVartypeString(pval).c_str()));
	mBuf.catln(strf("アドレス：0x%08X, 0x%08X", address_cast(pval->pt), address_cast(pval->master)));
	mBuf.catln(strf("サイズ：using %d of %d [byte]", pval->size, bufsize));
	mBuf.catCrlf();

	// 変数の内容に関する情報
	{
		auto const varinf = std::make_unique<CVarinfoTree>(mBuf.getLimit());
		varinf->addVar(name, pval);
		mBuf.cat(varinf->getString());
	}
	mBuf.catCrlf();

	// メモリダンプ
	mBuf.catDump(pMemBlock, static_cast<size_t>(bufsize));
	return;
}

//------------------------------------------------
// システム変数データから生成
//------------------------------------------------
void CVarinfoText::addSysvar(char const* name)
{
	auto const id = Sysvar_seek(name);
	if ( id == SysvarId_MAX ) return;

	mBuf.catln(strf("変数名：%s\t(システム変数)", name));
	mBuf.catln(strf("変数型：%s", hpimod::getHvp(SysvarData[id].type)->vartype_name));
	mBuf.catCrlf();

	{
		auto const varinf = std::make_unique<CVarinfoTree>(mBuf.getLimit());
		varinf->addSysvar(id);
		mBuf.catln(varinf->getString());
	}

	{
		void const* data; size_t size;
		Sysvar_getDumpInfo(id, data, size);
		mBuf.catDump(data, size);
	}
	return;
}

#if with_WrapCall
#include "module/map_iterator.h"
//------------------------------------------------
// 呼び出しデータから生成
// 
// @prm prmstk: nullptr => 引数未確定
//------------------------------------------------
void CVarinfoText::addCall(ModcmdCallInfo const& callinfo)
{
	auto const stdat = callinfo.stdat;
	auto const name = hpimod::STRUCTDAT_getName(stdat);
	mBuf.catln(
		(callinfo.fname == nullptr)
			? strf("関数名：%s", name)
			: strf("関数名：%s (#%d of %s)", name, callinfo.line, callinfo.fname)
	);

	// シグネチャ
#if 0
	{
		// 仮引数リストの文字列
		string s = "";

		//if ( stdat->prmmax == 0 ) s = "void";
		std::for_each(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat), [&](STRUCTPRM const& stprm) {
			if ( !s.empty() ) s += ", ";
			s += getMPTypeString(stprm.mptype);
		});
		mBuf.catln(strf("仮引数：(%s)", s.c_str()));
#else
	{
		auto const range = make_mapped_range(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat),
			[&](STRUCTPRM const& stprm) { return getMPTypeString(stprm.mptype); });
		mBuf.catln(strf("仮引数：(%s)", 
			join(range.begin(), range.end(), ", ").c_str()));
	}
#endif
	mBuf.catCrlf();

	if ( auto const prmstk = callinfo.getPrmstk() ) {
		// 変数の内容に関する情報
		auto const varinf = std::make_unique<CVarinfoTree>(mBuf.getLimit());
		varinf->addCall( stdat, prmstk );
		mBuf.cat(varinf->getString());
		mBuf.catCrlf();

		// メモリダンプ
		mBuf.catDump( prmstk, static_cast<size_t>(stdat->size) );

	} else {
		mBuf.catln("[展開中]");
	}
	return;
}

//------------------------------------------------
// 返値データから生成
//------------------------------------------------
void CVarinfoText::addResult( stdat_t stdat, void* ptr, vartype_t vtype, char const* name )
{
	int const bufsize = hpimod::getHvp(vtype)->GetSize( ptr_cast<PDAT*>(ptr) );
	
	mBuf.catln(strf( "関数名：%s", name ));
	mBuf.catCrlf();

	// 変数の内容に関する情報
	{
		auto const varinf = std::make_unique<CVarinfoTree>(mBuf.getLimit());
		varinf->addResult(name, ptr, vtype);
		mBuf.cat(varinf->getString());
	}
	mBuf.catCrlf();
	
	// メモリダンプ
	mBuf.catDump( ptr, static_cast<size_t>(bufsize) );
	return;
}

void CVarinfoText::addResult2( string const& text, char const* name )
{
	mBuf.catln(text);
	return;
}

#endif

//**********************************************************
//        下請け関数
//**********************************************************
/*
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
void CVarinfoText::cat( char const* string )
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
void CVarinfoText::catf( char const* format, ... )
{
	va_list   arglist;
	va_start( arglist, format );
	
	string s = vstrf( format, arglist );
	cat( s.c_str() );
	
	va_end( arglist );
	return;
}
//*/

//------------------------------------------------
// mptype の文字列を得る
// todo: hpimod に移動？
//------------------------------------------------
char const* getMPTypeString(int mptype)
{
	switch ( mptype ) {
		case MPTYPE_NONE:        return "none";
		case MPTYPE_STRUCTTAG:   return "structtag";

		case MPTYPE_LABEL:       return "label";
		case MPTYPE_DNUM:        return "double";
		case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING: return "str";
		case MPTYPE_INUM:        return "int";
		case MPTYPE_VAR:
		case MPTYPE_PVARPTR:				// #dllfunc
		case MPTYPE_SINGLEVAR:   return "var";
		case MPTYPE_ARRAYVAR:    return "array";
		case MPTYPE_LOCALVAR:    return "local";
		case MPTYPE_MODULEVAR:   return "thismod";//"modvar";
		case MPTYPE_IMODULEVAR:  return "modinit";
		case MPTYPE_TMODULEVAR:  return "modterm";

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
#endif
		default: return "unknown";
	}
}

#if 0 // 未使用
static char const* getModeString(varmode_t mode)
{
	return	(mode == HSPVAR_MODE_NONE) ? "無効" :
		(mode == HSPVAR_MODE_MALLOC) ? "実体" :
		(mode == HSPVAR_MODE_CLONE) ? "クローン" : "???";
}
#endif
static char const* getTypeQualifierFromMode(varmode_t mode)
{
	return (mode == HSPVAR_MODE_NONE) ? "!" :
		(mode == HSPVAR_MODE_MALLOC) ? "" :
		(mode == HSPVAR_MODE_CLONE) ? "&" : "<err>";
}

// 変数の型を表す文字列
string getVartypeString(PVal const* pval)
{
	size_t const maxDim = hpimod::PVal_maxDim(pval);

	string const arrayType =
		(maxDim == 0) ? "(empty)" :
		(maxDim == 1) ? makeArrayIndexString(1, &pval->len[1]) :
		strf("%s (%d in total)", makeArrayIndexString(maxDim, &pval->len[1]).c_str(), hpimod::PVal_cntElems(pval))
	;

	return strf("%s%s %s",
		hpimod::getHvp(pval->flag)->vartype_name,
		getTypeQualifierFromMode(pval->mode),
		arrayType.c_str()
	);
}
