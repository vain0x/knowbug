
#include <functional>
#include <memory>

#include "../hsp3plugin_custom.h"
#include "knowbugForHPI.h"

static HMODULE knowbug_getInstance()
{
	// cf: http://hwada.hatenablog.com/entry/20100214/1266073808
	// and http://cpplover.blogspot.jp/2010/02/blog-post_8598.html

	struct moduleDeleter {
		using pointer = HMODULE;
		void operator ()(HMODULE p) { FreeLibrary(p); }
	};
	using moduleHandle_t = std::unique_ptr< HMODULE, moduleDeleter >;
	
	// singleton (thread unsafe style)
	static moduleHandle_t stt_hKnowbug;

	if ( hpimod::isDebugMode() && !stt_hKnowbug ) {
		stt_hKnowbug.reset(LoadLibrary("hsp3debug.dll"));
		if ( !stt_hKnowbug ) {
#if _DEBUG
			MessageBox(nullptr, "knowbug の load に失敗しました。", "hpimod", MB_OK);
#endif
			puterror(HSPERR_EXTERNAL_EXECUTE);
		}
	}

	return stt_hKnowbug.get();
}

KnowbugVswMethods const* knowbug_getVswMethods()
{
	static KnowbugVswMethods const* kvswm { nullptr };

	if ( !kvswm ) {
		auto const hModule = knowbug_getInstance();
		if ( hModule ) {
			if ( auto const f = reinterpret_cast<knowbug_getVswMethods_t>(
				GetProcAddress(hModule, "_knowbug_getVswMethods@0")
				) ) {
				kvswm = f();
				if ( !kvswm ) { puterror(HSPERR_EXTERNAL_EXECUTE); }
			}
		}
	}
	return kvswm;
}
