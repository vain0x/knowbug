// スクリプト側との通信用

#ifdef with_Script

#include "D:/Docs/prg/cpp/MakeHPI/WrapCall/DbgWndMsg.h"

//#include <Windows.h>
#include <map>
#include <algorithm>

#include "main.h"
#include "with_Script.h"

static void initConnectWithScript();

static void setNodeAnnotation(char const* name, char const* msg);

/*
static void setStPrmNameBegin( char const* modname );
static void setStPrmNameEnd();
static void setStPrmName( int idx, char const* name );
typedef std::map<const stprm_t, std::string> stprm_names_t;
//*/

typedef std::map<std::string, std::string> annotation_t;

// ファイルスコープ変数
static annotation_t* stt_annotations = nullptr;
//static stprm_names_t* stt_stprm_names = nullptr;

//#define inst_of(var) std::remove_pointer<decltype(var)>::type

//------------------------------------------------
// デタッチ時
//------------------------------------------------
void termConnectWithScript()
{
	delete stt_annotations; stt_annotations = nullptr;
	//delete stt_stprm_names; stt_stprm_names = nullptr;
	return;
}

//------------------------------------------------
// スクリプト側からの返信
//------------------------------------------------
void initConnectWithScript()
{
	stt_annotations = new annotation_t();
	//stt_stprm_names = new stprm_names_t();
	return;
}

//------------------------------------------------
// ノード注釈
//------------------------------------------------
void setNodeAnnotation( char const* name, char const* msg )
{
	if ( !stt_annotations ) return;
	stt_annotations->insert(annotation_t::value_type(name, msg));
	return;
}

char const* getNodeAnnotation( char const* name )
{
	if ( !stt_annotations ) return nullptr;
	auto const iter = stt_annotations->find( name );
	return ( iter != stt_annotations->end() ) ? iter->second.c_str() : nullptr;
}

// 後方互換
char const* getStPrmName(const stprm_t stprm)
{
	return (stprm->mptype == MPTYPE_MODULEVAR) ? "thismod" : nullptr;
}
/*
//------------------------------------------------
// 構造体パラメータの識別子を設定する
// 
// @ begin で対象となるモジュールやユーザ定義コマンドを選択し、
// @	その後に StPrmName を連続で呼び出す。
//------------------------------------------------
// setter
void setStPrmName( const stprm_t stprm, char const* name )
{
	if ( !stt_stprm_names ) return;
	stt_stprm_names->insert(stprm_names_t::value_type(stprm, name));
	
//	dbgmsg(strf("set stprm {subid %d, mptype %d, offset %d},\nname %s", stprm->subid, stprm->mptype, stprm->offset, name).c_str());
	return;
}

// getter
char const* getStPrmName( const stprm_t stprm )
{
	if ( stprm->mptype == MPTYPE_MODULEVAR ) return "thismod";
	if ( !stt_stprm_names ) return nullptr;
	auto const iter = stt_stprm_names->find( stprm );

//	dbgmsg(strf("get stprm {subid %d, mptype %d, offset %d},\nname %s", stprm->subid, stprm->mptype, stprm->offset, (iter == stt_stprm_names->end() ? "" : iter->second.c_str())).c_str());
	return ( iter != stt_stprm_names->end() ) ? iter->second.c_str() : nullptr;
}

static const stdat_t stt_stdatTarget;	// 現在識別子設定中のもの

// 特定の名前の構造体(STRUCTDAT)を探す (完全一致なので小文字でなければヒットしない)
auto SeekStDat(char const* name) -> const stdat_t {
	const stdat_t stdat = ctx->mem_finfo;
	for ( size_t i = 0; i < ctx->hsphed->max_finfo / sizeof(STRUCTDAT); ++ i ) {
		if ( stdat[i].nameidx >= 0 && !std::strcmp( STRUCTDAT_getName(&stdat[i]), name ) ) return &stdat[i];
	}
	return nullptr;
}

void setStPrmNameBegin( char const* nameStDat )
{
	if ( stt_stdatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 他の対象の操作中

	const size_t len = std::strlen(nameStDat);
	std::string name( len + 1, '\0' );
	std::transform( nameStDat, nameStDat + len, name.begin(), tolower );	// 小文字化
	
	stt_stdatTarget = SeekStDat( name.c_str() );
	if ( !stt_stdatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 見つからなかった
	return;
}

void setStPrmNameEnd()
{
	if ( !stt_stdatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 無駄な呼び出し
	stt_stdatTarget = nullptr;
	return;
}

void setStPrmName( int idx, char const* name )
{
	if ( !stt_stdatTarget ) throw HSPERR_ILLEGAL_FUNCTION;
	const size_t prmidx = stt_stdatTarget->prmindex;

	switch ( ctx->mem_minfo[prmidx].mptype ) {
		case MPTYPE_STRUCTTAG:  case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR: case MPTYPE_TMODULEVAR:
			idx ++;		// structtag や thismod 引数の分は飛ばす
			break;
	}
	if ( !(0 <= idx && idx < stt_stdatTarget->prmindex) ) throw HSPERR_ILLEGAL_FUNCTION;
	return setStPrmName( &ctx->mem_minfo[prmidx + idx], name );
}
//*/

EXPORT void WINAPI knowbug_greet()
{
	initConnectWithScript();
	return;
}

/*
EXPORT void WINAPI knowbug_namePrms(char const* nameStDat,
	char const* p1, char const* p2, char const* p3, char const* p4,
	char const* p5, char const* p6, char const* p7, char const* p8,
	char const* p9, char const* p10, char const* p11, char const* p12,
	char const* p13, char const* p14, char const* p15)
{
	setStPrmNameBegin( nameStDat );
	char const* const names[] = { p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15 };
	for ( int i = 0; i < 15; ++ i ) {
		if ( names[i] == nullptr ) continue;
		setStPrmName( i, names[i] );
	}
	setStPrmNameEnd();
}
//*/
/*
	char const* nameStDat = exinfo->HspFunc_prm_gets();
	setStPrmNameBegin( nameStDat );
	
	PVal*& mpval = *exinfo->mpval;
	for ( int i = 0;; i ++ ) {
		const int ok = exinfo->HspFunc_prm_get();
		if ( ok <= PARAM_END ) {
			if ( ok == PARAM_DEFAULT ) continue; else break;
		}
		if ( mpval->flag != HSPVAR_FLAG_STR ) exinfo->HspFunc_puterror( HSPERR_TYPE_MISMATCH );
		setStPrmName( i, mpval->pt );
	}
	return;
//*/

#endif
