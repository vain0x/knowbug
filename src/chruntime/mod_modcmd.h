// chruntime.module - ModuleCmd

#ifndef IG_MODULE_MODCMD_H
#define IG_MODULE_MODCMD_H

#include <vector>
#include "hsp3struct.h"
#include "mod_vector.h"

// @global g_ctx;

//##############################################################################
//                型定義
//##############################################################################
struct PrmStk
{
	Vector* argv;
	uint argc;			// 保持する実引数の数 ( PrmStk 生成中に GC が利用する )
	STRUCTDAT* pStDat;
	STRUCTPRM* pStPrm;
	
	// 呼び出しリンク
	PrmStk* pUpper;
	PrmStk* pLower;
};

//##############################################################################
//                関数宣言
//##############################################################################

//**********************************************************
//        PrmStk
//**********************************************************
extern void prmstk_register(void);
extern PrmStk* new_prmstk( STRUCTDAT* pStDat, uint argc = 0, const Vector* src = NULL );

//extern void PrmStk_push( PrmStk* prmstk, c_valptr_t val );
extern void PrmStk_push( PrmStk* prmstk, ScrValue* pVal );
extern void PrmStk_push( PrmStk* prmstk, c_valptr_t src, vartype_t vt );

extern ModInst*  PrmStk_getThismod( PrmStk* prmstk );
extern ScrValue* PrmStk_at( PrmStk* prmstk, STRUCTPRM* pStPrm );

#endif
