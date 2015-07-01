// WrapCall header

#ifndef IG_WRAP_CALL_H
#define IG_WRAP_CALL_H

#include "hsp3plugin_custom.h"

struct ModcmdCallInfo;
struct STRUCTDAT;

namespace WrapCall
{

struct WrapCallData;

// 変数宣言
extern WrapCallData* g_pWrapCallData;

// 関数宣言
extern void init(void);
extern void term(void);

extern void bgnCall( STRUCTDAT* pStDat );
extern void endCall( void* p = NULL, vartype_t vt = HSPVAR_FLAG_NONE );

};

#endif
