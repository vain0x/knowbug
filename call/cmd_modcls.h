// Call(ModCls) - Command header

#ifndef IG_CALL_MODCLS_COMMAND_H
#define IG_CALL_MODCLS_COMMAND_H

#include "hsp3plugin_custom.h"

extern bool ModCls_IsWorking();

extern void ModCls_Init();
extern void ModCls_Term();

extern void ModCls_Register();

extern void ModCls_Newmod();
extern int  ModCls_Newmod( void** ppResult, bool bSysvar = false );
extern void ModCls_Delmod();
extern int  ModCls_Nullmod( void** ppResult );
extern void ModCls_Dupmod();
extern int  ModCls_Dupmod( void** ppResult );
extern int  ModCls_Identity( void** ppResult );
extern int  ModCls_Name( void** ppResult );

extern int  ModInst_Cls( void** ppResult );
extern int  ModInst_ClsName( void** ppResult );
extern int  ModInst_Identify( void** ppResult );

extern int  ModCls_This( void** ppResult );

enum OpFlag
{
	OpFlag_Calc    = 0x0000,
	OpFlag_CnvTo   = 0x0100,
	OpFlag_Sp      = 0x0200,

	// ‚»‚Ì‘¼
	OpId_Dup    = OpFlag_Sp | 0,
	OpId_Cmp    = OpFlag_Sp | 1,
	OpId_Method = OpFlag_Sp | 2,

	OpId_Max
};

#endif
