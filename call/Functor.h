// 関数子オブジェクト

#ifndef IG_CLASS_FUNCTOR_H
#define IG_CLASS_FUNCTOR_H

#include "IFunctor.h"
#include "Managed.h"

using namespace hpimod;

namespace hpimod {
namespace detail {
	template<>
	struct DefaultCtorDtor<IFunctor> {
		static inline void defaultCtor(IFunctor* p) { assert(false); }
		static inline void defaultDtor(IFunctor& self) { self.~IFunctor(); }
	};
}
}

using functor_t = Managed<IFunctor, true>;

#include "CLabelFunc.h"
#include "CDeffunc.h"

/*
#include "CBound.h"
#include "CCoRoutine.h"
#include "CLambda.h"
#include "CStreamCaller.h"
#include "CModClsCtor.h"
//*/

static functor_t Functor_New(label_t lb)
{
	return functor_t::make<CLabelFunc>(lb);
}

static functor_t Functor_New(int axcmd)
{
	assert(AxCmd::isOk(axcmd));

	switch ( AxCmd::getType(axcmd) ) {
		case TYPE_LABEL: return functor_t::make<CLabelFunc>(axcmd);
		case TYPE_MODCMD: return functor_t::make<CDeffunc>(axcmd);
		default:
			puterror(HSPERR_TYPE_MISMATCH);
	}
}

#endif
