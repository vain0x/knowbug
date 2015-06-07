// customized hsp3plugin header (uedai)

#ifndef IG_HSP3PLUGIN_CUSTOM_H
#define IG_HSP3PLUGIN_CUSTOM_H

#include <windows.h>
#undef max
#undef min

#include "hsp3plugin.h"
#undef stat	// いくつかの標準ライブラリと衝突してしまうので

#undef puterror
#define puterror(err) do { dbgout("puterror " #err " in hpi"); exinfo->HspFunc_puterror(err); throw err; } while(false)

#include "./basis.h"
#include "./vartype_traits.h"

#endif
