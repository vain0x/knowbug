// WrapCall - main.h

#ifndef IG_CALOG_MAIN_H
#define IG_CALOG_MAIN_H

#define WIN32_LEAN_AND_MEAN		// <commdlg.h> を include しない
#include <windows.h>

//#include <cstdio>
//#include <cstdlib>
//#include <cstring>

//#include "strbuf.h"
#include "hsp3plugin_custom.h"

// 変数宣言
extern int g_pluginType_WrapCall;

namespace WrapCallCmdId
{
	const int
		ByRef = 0x210
	;
};

#endif
