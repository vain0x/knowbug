// vartype - functor

#ifndef IG_VARTYPE_FUNCTOR_H
#define IG_VARTYPE_FUNCTOR_H

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"
#include "vp_template.h"

#include "Functor.h"

// 変数
extern vartype_t g_vtFunctor;
extern HspVarProc* g_hvpFunctor;

// 関数
extern void HspVarFunctor_init(HspVarProc* vp);

// vartype tag
struct functor_tag
	: public NativeVartypeTag<functor_t>
{
	static vartype_t vartype() { return g_vtFunctor; }
};

// VtTraits<> の特殊化
namespace hpimod
{
	template<> struct VtTraits<functor_tag> : public VtTraitsBase<functor_tag>
	{
	};
}

using FunctorTraits = hpimod::VtTraits<functor_tag>;

// 返値設定関数
extern functor_t g_resFunctor;		// 終了時、静的変数などより先に解体する
static int SetReffuncResult( void** ppResult, functor_t const& src );

#endif
