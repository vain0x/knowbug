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

namespace Functor
{
	extern functor_t const& New(int axcmd);
	extern functor_t const& New(label_t lb);

	template<typename TFunctor, typename ...Args>
	static functor_t New(Args&&... args) { return functor_t::make<TFunctor>(std::forward<Args>(args)...); }

	extern void Terminate();
}

#endif
