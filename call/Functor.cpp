
#include <map>
#include "Functor.h"

// キャッシュ
static std::map<int /*axcmd*/, functor_t> stt_functorCache;

//------------------------------------------------
// キャッシュされる functor の生成
//------------------------------------------------
static functor_t const& Functor_NewCache(int axcmd)
{
	assert(AxCmd::isOk(axcmd));

	auto const iter = stt_functorCache.find(axcmd);
	if ( iter != stt_functorCache.end() ) {
		return iter->second;

	} else {
		functor_t f { nullptr };

		switch ( AxCmd::getType(axcmd) ) {
			case TYPE_LABEL:  f = functor_t::make<CLabelFunc>(axcmd);
			case TYPE_MODCMD: f = functor_t::make<CDeffunc>(axcmd);
			default:
				puterror(HSPERR_TYPE_MISMATCH);
		}
		return stt_functorCache.insert({ axcmd, f }).first->second;
	}
}

//------------------------------------------------
// functor の生成
//------------------------------------------------
functor_t Functor_New(int axcmd)
{
	return Functor_NewCache(axcmd);
}

functor_t const& Functor_New(label_t lb)
{
	return Functor_New(AxCmd::make(TYPE_LABEL, hpimod::getOTPtr(lb)));
}

//------------------------------------------------
//------------------------------------------------

//------------------------------------------------
//------------------------------------------------
void Terminate()
{
	stt_functorCache.clear();
}
