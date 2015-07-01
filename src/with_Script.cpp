// スクリプト側との通信用

#ifdef with_Script

#include <Windows.h>
#include <map>
#include <algorithm>

#include "main.h"
#include "with_Script.h"

#include "../../../../../../MakeHPI/WrapCall/DbgWndMsg.h"

static void initConnectWithScript();

static void setNodeAnnotation(const char* name, const char* msg);

static void setStPrmNameBegin( const char* modname );
static void setStPrmNameEnd();
static void setStPrmName( int idx, const char* name );

// ファイルスコープ変数
static std::map<std::string, std::string>* stt_annotations = nullptr;
static std::map<const STRUCTPRM*, std::string>* stt_stprm_names = nullptr;

#define inst_of(var) std::remove_pointer<decltype(var)>::type

//------------------------------------------------
// デタッチ時
//------------------------------------------------
void termConnectWithScript()
{
	delete stt_annotations; stt_annotations = nullptr;
	delete stt_stprm_names; stt_stprm_names = nullptr;
	return;
}

//------------------------------------------------
// スクリプト側からの返信
//------------------------------------------------
void initConnectWithScript()
{
	stt_annotations = new inst_of(stt_annotations)();
	stt_stprm_names = new inst_of(stt_stprm_names)();
	return;
}

//------------------------------------------------
// ノード注釈
//------------------------------------------------
void setNodeAnnotation( const char* name, const char* msg )
{
	if ( !stt_annotations ) return;
	stt_annotations->insert( inst_of(stt_annotations)::value_type( name, msg ) );
	return;
}

const char* getNodeAnnotation( const char* name )
{
	if ( !stt_annotations ) return nullptr;
	auto const iter = stt_annotations->find( name );
	return ( iter != stt_annotations->end() ) ? iter->second.c_str() : nullptr;
}

//------------------------------------------------
// 構造体パラメータの識別子を設定する
// 
// @ begin で対象となるモジュールやユーザ定義コマンドを選択し、
// @	その後に StPrmName を連続で呼び出す。
//------------------------------------------------
// setter
void setStPrmName( const STRUCTPRM* pStPrm, const char* name )
{
	if ( !stt_stprm_names ) return;
	stt_stprm_names->insert( inst_of(stt_stprm_names)::value_type( pStPrm, name ) );
	
//	dbgmsg(strf("set pStPrm {subid %d, mptype %d, offset %d},\nname %s", pStPrm->subid, pStPrm->mptype, pStPrm->offset, name).c_str());
	return;
}

// getter
const char* getStPrmName( const STRUCTPRM* pStPrm )
{
	if ( pStPrm->mptype == MPTYPE_MODULEVAR ) return "thismod";
	if ( !stt_stprm_names ) return nullptr;
	auto const iter = stt_stprm_names->find( pStPrm );

//	dbgmsg(strf("get pStPrm {subid %d, mptype %d, offset %d},\nname %s", pStPrm->subid, pStPrm->mptype, pStPrm->offset, (iter == stt_stprm_names->end() ? "" : iter->second.c_str())).c_str());
	return ( iter != stt_stprm_names->end() ) ? iter->second.c_str() : nullptr;
}

static const STRUCTDAT* stt_pStDatTarget;	// 現在識別子設定中のもの

// 特定の名前の構造体(STRUCTDAT)を探す (完全一致なので小文字でなければヒットしない)
auto SeekStDat(const char* name) -> const STRUCTDAT* {
	const STRUCTDAT* pStDat = ctx->mem_finfo;
	for ( size_t i = 0; i < ctx->hsphed->max_finfo / sizeof(STRUCTDAT); ++ i ) {
		if ( pStDat[i].nameidx >= 0 && !std::strcmp( STRUCTDAT_getName(&pStDat[i]), name ) ) return &pStDat[i];
	}
	return nullptr;
}

void setStPrmNameBegin( const char* nameStDat )
{
	if ( stt_pStDatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 他の対象の操作中

	const size_t len = std::strlen(nameStDat);
	std::string name( len + 1, '\0' );
	std::transform( nameStDat, nameStDat + len, name.begin(), tolower );	// 小文字化
	
	stt_pStDatTarget = SeekStDat( name.c_str() );
	if ( !stt_pStDatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 見つからなかった
	return;
}

void setStPrmNameEnd()
{
	if ( !stt_pStDatTarget ) throw HSPERR_ILLEGAL_FUNCTION;	// 無駄な呼び出し
	stt_pStDatTarget = nullptr;
	return;
}

void setStPrmName( int idx, const char* name )
{
	if ( !stt_pStDatTarget ) throw HSPERR_ILLEGAL_FUNCTION;
	const size_t prmidx = stt_pStDatTarget->prmindex;

	switch ( ctx->mem_minfo[prmidx].mptype ) {
		case MPTYPE_STRUCTTAG:  case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR: case MPTYPE_TMODULEVAR:
			idx ++;		// structtag や thismod 引数の分は飛ばす
			break;
	}
	if ( !(0 <= idx && idx < stt_pStDatTarget->prmindex) ) throw HSPERR_ILLEGAL_FUNCTION;
	return setStPrmName( &ctx->mem_minfo[prmidx + idx], name );
}

EXPORT void WINAPI knowbug_greet()
{
	initConnectWithScript();
	return;
}

EXPORT void WINAPI knowbug_namePrms(const char* nameStDat,
	const char* p1, const char* p2, const char* p3, const char* p4,
	const char* p5, const char* p6, const char* p7, const char* p8,
	const char* p9, const char* p10, const char* p11, const char* p12,
	const char* p13, const char* p14, const char* p15)
{
	setStPrmNameBegin( nameStDat );
	const char* const names[] = { p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15 };
	for ( int i = 0; i < 15; ++ i ) {
		if ( names[i] == nullptr ) continue;
		setStPrmName( i, names[i] );
	}
	setStPrmNameEnd();
/*
	const char* nameStDat = exinfo->HspFunc_prm_gets();
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
}

#endif
