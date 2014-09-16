// call - main.h

#ifndef IG_INTERFACE_CALL_H
#define IG_INTERFACE_CALL_H

//#define WIN32_LEAN_AND_MEAN		// <commdlg.h> Ç include ÇµÇ»Ç¢
//#include <windows.h>

//#include <cstdio>
//#include <cstdlib>
//#include <cstring>

//#include "strbuf.h"
//#include "hsp3plugin_custom.h"

extern int g_pluginType_call;

namespace CallCmdId
{
	int const
		ByRef     = 0x210,
		ByThismod = 0x211,
		ByDef     = 0x212,
		NoBind    = 0x213,
		PrmOf     = 0x214,
		ValOf     = 0x215,
		NoCall    = 0x216,

		RetVal     = 0x003,
		ArgV       = 0x101,
		Lambda     = 0x12A,
		LambdaBody = 0x070,	// ì‡ïîóp
		BgnOfPrm   = 0xFD0	//Å©Ç»Ç…Ç±ÇÍÅH
	;
};

#endif
