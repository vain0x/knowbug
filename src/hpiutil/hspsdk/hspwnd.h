
//
//	HspWnd,Bmscr(BMSCR) struct define
//
#ifndef __hspwnd_h
#define __hspwnd_h

#ifdef HSPDISH
#include "../hsp3dish/hspwnd.h"
#else

#ifdef HSPWIN
#include "win32gui/hspwnd_win.h"
#endif

#ifdef HSPLINUX
#include "linux/hspwnd_linux.h"
#endif

#endif

#endif
