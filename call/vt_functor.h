// vartype - functor

#ifndef IG_VARTYPE_FUNCTOR_H
#define IG_VARTYPE_FUNCTOR_H

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"

#include "CFunctor.h"

// 変数宣言
extern vartype_t HSPVAR_FLAG_FUNCTOR;
extern HspVarProc* g_pHvpFunctor;

// 関数宣言
extern void HspVarFunctor_init(HspVarProc* vp);

// PVal::pt に配列状に管理されるオブジェクト
using functor_t = CFunctor;

// 返値設定関数
extern functor_t g_resFunctor;		// 終了時、静的変数などより先に解体する
static int SetReffuncResult( void** ppResult, functor_t const& src )
{
	g_resFunctor = src;
	*ppResult = &g_resFunctor;
	return HSPVAR_FLAG_FUNCTOR;
}
//FTM_SetReffuncResult( functor_t, HSPVAR_FLAG_FUNCTOR );

#endif
