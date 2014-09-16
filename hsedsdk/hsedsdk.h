// hsedsdk for C++

// based on (ver 3.31RC1)
// edit by uedai

#ifndef IG_HSEDSDK_H
#define IG_HSEDSDK_H

#include <Windows.h>

// Win32API 定数(ウィンドウ メッセージを除く)
/*
#define PROCESS_ALL_ACCESS    0x001F0FFF
#define DUPLICATE_SAME_ACCESS 0x00000002
#define  CF_OEMTEXT       $00000007
//*/

//
// 文字列定数
#define HSED_INTERFACE_NAME "HspEditorInterface"

//
// ウィンドウ メッセージ
//#define WM_APP 0x00008000
#define _HSED_GETVER          (WM_APP + 0x000)
#define _HSED_GETWND          (WM_APP + 0x100)
#define _HSED_GETPATH         (WM_APP + 0x101)

#define _HSED_GETTABCOUNT     (WM_APP + 0x200)
#define _HSED_GETTABID        (WM_APP + 0x201)
#define _HSED_GETFOOTYID      (WM_APP + 0x202)
#define _HSED_GETACTTABID     (WM_APP + 0x203)
#define _HSED_GETACTFOOTYID   (WM_APP + 0x204)

#define _HSED_CANCOPY         (WM_APP + 0x300)
#define _HSED_CANPASTE        (WM_APP + 0x301)
#define _HSED_CANUNDO         (WM_APP + 0x302)
#define _HSED_CANREDO         (WM_APP + 0x303)
#define _HSED_GETMODIFY       (WM_APP + 0x304)

#define _HSED_COPY            (WM_APP + 0x310)
#define _HSED_CUT             (WM_APP + 0x311)
#define _HSED_PASTE           (WM_APP + 0x312)
#define _HSED_UNDO            (WM_APP + 0x313)
#define _HSED_REDO            (WM_APP + 0x314)
#define _HSED_INDENT          (WM_APP + 0x315)
#define _HSED_UNINDENT        (WM_APP + 0x316)
#define _HSED_SELECTALL       (WM_APP + 0x317)

#define _HSED_SETTEXT         (WM_APP + 0x320)
#define _HSED_GETTEXT         (WM_APP + 0x321)
#define _HSED_GETTEXTLENGTH   (WM_APP + 0x322)
#define _HSED_GETLINES        (WM_APP + 0x323)
#define _HSED_SETSELTEXT      (WM_APP + 0x324)
#define _HSED_GETSELTEXT      (WM_APP + 0x325)
#define _HSED_GETLINETEXT     (WM_APP + 0x326)
#define _HSED_GETLINELENGTH   (WM_APP + 0x327)
#define _HSED_GETLINECODE     (WM_APP + 0x328)

#define _HSED_SETSELA         (WM_APP + 0x330)
#define _HSED_SETSELB         (WM_APP + 0x331)
#define _HSED_GETSELA         (WM_APP + 0x332)
#define _HSED_GETSELB         (WM_APP + 0x333)

#define _HSED_GETCARETLINE    (WM_APP + 0x340)
#define _HSED_GETCARETPOS     (WM_APP + 0x341)
#define _HSED_GETCARETTHROUGH (WM_APP + 0x342)
#define _HSED_GETCARETVPOS    (WM_APP + 0x343)
#define _HSED_SETCARETLINE    (WM_APP + 0x344)
#define _HSED_SETCARETPOS     (WM_APP + 0x345)
#define _HSED_SETCARETTHROUGH (WM_APP + 0x346)

#define _HSED_SETMARK         (WM_APP + 0x350)
#define _HSED_GETMARK         (WM_APP + 0x351)
#define _HSED_SETHIGHLIGHT    (WM_APP + 0x352)

//
// HSED_GETVER用の定数
#define HGV_PUBLICVER    0
#define HGV_PRIVATEVER   1
#define HGV_HSPCMPVER    2
#define HGV_FOOTYVER     3
#define HGV_FOOTYBETAVER 4

//
// HSED_GETWND用の定数
#define HGW_MAIN      0
#define HGW_CLIENT    1
#define HGW_TAB       2
#define HGW_EDIT      3
#define HGW_TOOLBAR   4
#define HGW_STATUSBAR 5

//
// マクロ
#define hsed_getmajorver(n) ((n) >> 16 & 0xFFFF)
#define hsed_getminorver(n) ((n) >> 8 & 0xFF)
#define hsed_getbetaver(n)  ((n) & 0xFF)

void hsed_uninitduppipe();
bool hsed_initduppipe( int nSize );
bool hsed_capture();
bool hsed_exist();
//void hsed_getver( var ret, int nType );
//void hsed_getwnd( var ret, int nType, int nID );
int  hsed_getpath( char* ret, int nTabID );
//void hsed_cnvverstr( int nVersion );
int  hsed_gettabcount( int& ret );
/*
void hsed_gettabid( var ret, int nFootyID );
void hsed_getfootyid( var ret, int nTabID );
void hsed_cancopy( var ret, int nFootyID );
void hsed_canpaste( var ret );
void hsed_canundo( var ret, int nFootyID );
void hsed_canredo( var ret, int nFootyID );
void hsed_getmodify( var ret, int nFootyID );
void hsed_copy( int nFootyID );
void hsed_cut( int nFootyID );
void hsed_paste( int nFootyID );
void hsed_undo( int nFootyID );
void hsed_redo( int nFootyID );
void hsed_indent( int nFootyID );
void hsed_unindent( int nFootyID );
void hsed_selectall( int nFootyID );
void hsed_gettextlength( var ret, int nFootyID );
void hsed_getlines( var ret, int nFootyID );
void hsed_getlinelength( var ret, int nFootyID, int nLine );
void hsed_getlinecode( var ret, int nFootyID );
void hsed_gettext( var ret, int nFootyID );
void hsed_sendtext_msg( int nFootyID, int msg, var sText );
void hsed_settext( int nFootyID, str sText );
void hsed_getactfootyid( var ret );
void hsed_getacttabid( var ret );
void hsed_sendstr( var _p1 );
//*/

#endif
