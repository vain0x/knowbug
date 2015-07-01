// WrapCall - type modcmd header

#ifndef IG_TYPE_MODCMD_H
#define IG_TYPE_MODCMD_H

#include "hsp3plugin_custom.h"
#include "ModcmdCallInfo.h"

// 変数宣言

// 関数宣言
extern void  modcmd_init( HSP3TYPEINFO* info );
extern void  modcmd_term( HSP3TYPEINFO* info );
extern int   modcmd_cmdfunc( int cmd );
extern void* modcmd_reffunc( int* type_res, int cmd );

#endif
