// opex - iface

#ifndef IG_OPEX_IFACE_H
#define IG_OPEX_IFACE_H

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "hsp3plugin.h"

EXPORT void WINAPI hpi_opex( HSP3TYPEINFO* info );

#endif
