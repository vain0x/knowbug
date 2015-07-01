
#ifdef HSPDEBUG

#include "mod_debug.h"

#include <cstdarg>
#include <cstdio>
#include <windows.h>

//------------------------------------------------
// メッセージボックスの表示
// 
// @ 書式文字列つき
//------------------------------------------------
int CMsgboxf::operator()(const char* pFormat, ...)
{
	static char stt_buf[1024];
	
	va_list  arglist;
	va_start(arglist, pFormat);
	vsprintf(stt_buf, pFormat, arglist);
	va_end(arglist);
	
	return MessageBox( 0, stt_buf, "dbgmsg", MB_OK );
}

#endif
