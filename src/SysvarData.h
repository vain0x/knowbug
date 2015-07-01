// システム変数データ

#ifndef IG_SYSVAR_DATA_H
#define IG_SYSVAR_DATA_H

#include "hsp3/hsp3struct.h"

struct StSysvarData
{
	const char *name;
	int        type;		// -1 => 特殊
};

//**********************************************************
//        システム変数データ
//**********************************************************
static StSysvarData SysvarData[]
= {
	{ "stat",    HSPVAR_FLAG_INT },
	{ "refstr",  HSPVAR_FLAG_STR },
	{ "refdval", HSPVAR_FLAG_DOUBLE },
	{ "thismod", HSPVAR_FLAG_STRUCT },
	{ "cnt",     HSPVAR_FLAG_INT },
	{ "iparam",  HSPVAR_FLAG_INT },
	{ "wparam",  HSPVAR_FLAG_INT },
	{ "lparam",  HSPVAR_FLAG_INT },
	{ "strsize", HSPVAR_FLAG_INT },
	{ "looplev", HSPVAR_FLAG_INT },
	{ "sublev",  HSPVAR_FLAG_INT },
	{ "err",     HSPVAR_FLAG_INT },
//	{ "mousex",  HSPVAR_FLAG_INT },
//	{ "mousey",  HSPVAR_FLAG_INT },
//	{ "mousew",  HSPVAR_FLAG_INT },
//	{ "ginfo",   HSPVAR_FLAG_INT }
//	{ "dirinfo", HSPVAR_FLAG_STR }
};

static const int SysvarCount = ( sizeof(SysvarData) / sizeof(SysvarData[0]) );

enum SysvarId
{
	SysvarId_Stat = 0,		// stat
	SysvarId_Refstr,		// refstr
	SysvarId_Refdval,		// refdval
	SysvarId_Thismod,		// thismod
	SysvarId_Cnt,			// cnt
	SysvarId_IParam,		// iparam
	SysvarId_WParam,		// wparam
	SysvarId_LParam,		// lparam
	SysvarId_StrSize,		// strsize
	SysvarId_Looplev,		// looplev
	SysvarId_Sublev,		// sublev
	SysvarId_Err,			// err
	
	// GUI
	SysvarId_MouseX,		// mousex
	SysvarId_MouseY,		// mousey
	SysvarId_MouseW,		// mousew
//	SysvarId_GInfo,			// ginfo
//	SysvarId_DirInfo,		// dirinfo
	SysvarId_MAX,
};

//**********************************************************
//        関数宣言
//**********************************************************
extern int getSysvarIndex( const char *name );

#endif
