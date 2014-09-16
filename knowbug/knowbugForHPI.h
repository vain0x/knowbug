// knowbug header for hpi

#ifndef IG_KNOWBUG_FOR_HSP_PLUGIN_H
#define IG_KNOWBUG_FOR_HSP_PLUGIN_H

#include <Windows.h>
#include "D:/Docs/prg/cpp/Project/knowbug/ExVardataString.h"

extern HMODULE g_hKnowbug;

extern void terminateKnowbugForHPI();
extern KnowbugVswMethods const* knowbug_getVswMethods();

#endif
