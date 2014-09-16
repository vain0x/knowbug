
#include "knowbugForHPI.h"

HMODULE g_hKnowbug;

extern void terminateKnowbugForHPI()
{
	if ( g_hKnowbug ) {
		FreeLibrary(g_hKnowbug); g_hKnowbug = nullptr;
	}
	return;
}

static KnowbugVswMethods const* kvswm;
extern KnowbugVswMethods const* knowbug_getVswMethods()
{
	if ( !g_hKnowbug ) {
		if ( g_hKnowbug = LoadLibrary("hsp3debug.dll") ) {
			if ( auto const f = reinterpret_cast<knowbug_getVswMethods_t>(
				GetProcAddress(g_hKnowbug, "_knowbug_getVswMethods@0")
				) ) {
				kvswm = f();
			}
		}
	}
	return kvswm;
}
