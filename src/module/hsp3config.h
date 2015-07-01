// clhsp - config

#ifndef IG_HSP3CONFIG_H
#define IG_HSP3CONFIG_H

//		システム関連ラベル
//
#define HSPTITLE "clhsp ver."
#define HSPVER   "3.2"
#define mvscode 6		// minor version code
#define vercode 0x3206	// version code

#define RunErr_HANDLE		// HSPエラー例外を有効にします
#define SYSERR_HANDLE		// システムエラー例外を有効にします

//
//		移植用のラベル
//
#define JPN			// IME use flag
#define JPNMSG		// japanese message flag

//
//	Debug mode functions
//
#define HSPDEBUGLOG	// Debug Log Version

//		Debug Window Message Buffer Size
//
#define DbgWnd_SelSize 0x10000
#define DbgWnd_MsgSize 0x10000
#define dbsel_size DbgWnd_SelSize
#define dbmes_size DbgWnd_MsgSize

//
//		以下のラベルはコンパイルオプションで設定されます
//
//#define HSPWIN		// Windows(WIN32) version flag
//#define HSPWINGUI		// Windows(WIN32) version flag
//#define HSPMAC		// Macintosh version flag
//#define HSPLINUX		// Linux(CLI) version flag
//#define HSPLINUXGUI	// Linux(GUI) version flag

//#define HSPDEBUG	// Debug version flag

//
//		移植用の定数
//
#if defined( HSPWIN )
# define HSP_MAX_PATH	260
# define HSP_PATH_SEPARATOR '\\'
#elif defined( HSPLINUX )
# define HSP_MAX_PATH	256
# define HSP_PATH_SEPARATOR '/'
#endif

#endif
