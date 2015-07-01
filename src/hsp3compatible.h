// hsp3 との下位互換向けデータ

#ifndef IG_HSP3COMPATIBLE_H
#define IG_HSP3COMPATIBLE_H

#include "chruntime/hspvar_core.h"

//##############################################################################
//                宣言部 : 削除された定義
//##############################################################################
#ifdef clhsp

// hsp のみ
struct MPModVarData;
struct FlexValue;

#endif

#ifdef clhsp
typedef MPModInst MPThismod;
typedef ModInst   ModclsInst;
#else
typedef MPModVarData MPThismod;
typedef FlexValue    ModclsInst;
#endif

#ifdef clhsp
//------------------------------------------------
// CALCCODE_*
//------------------------------------------------
enum CALCCODE
{
	CALCCODE_ADD = 0,
	CALCCODE_SUB,
	CALCCODE_MUL,
	CALCCODE_DIV,
	CALCCODE_MOD,
	CALCCODE_AND,
	CALCCODE_OR,
	CALCCODE_XOR,
	CALCCODE_EQ,
	CALCCODE_NE,
	CALCCODE_GT,
	CALCCODE_LT,
	CALCCODE_GTEQ,
	CALCCODE_LTEQ,
	CALCCODE_RR,
	CALCCODE_LR,
	CALCCODE_MAX
};
#endif

//##############################################################################
//                定義部 : 互換をとれるマクロ
//##############################################################################
#ifdef clhsp
# define PValLength(pHvp, pval, idx) (pHvp->Length((pval), (idx)))
# define PValLengthList(pHvp, pval) { PValLength(pHvp, pval, 1), PValLength(pHvp, pval, 2), PValLength(pHvp, pval, 3) }
#else
# define PValLength(pHvp, pval, idx) ((pval)->len[(idx)])
# define PValLengthList(pHvp, pval) { PValLength(pHvp, pval, 1), PValLength(pHvp, pval, 2), PValLength(pHvp, pval, 3), PValLength(pHvp, pval, 4) }
#endif

#ifdef clhsp
# define BracketIdxL "["
# define BracketIdxR "]"
#else
# define BracketIdxL "("
# define BracketIdxR ")"
#endif

#ifdef clhsp
inline STRUCTPRM* getModclsStPrm( HSPCTX* ctx, ModInst* p ) { return &ctx->mem_minfo[p->id_minfo]; }
inline STRUCTDAT* getModclsStDat( HSPCTX* ctx, ModInst* p ) { return &ctx->mem_finfo[getModclsStPrm(ctx, p)->subid]; }
inline void* getModclsMember( ModInst* p ) { return p->members; }
//# define getModclsStDat( pHspCtx, pInst ) (&(pHspCtx)->mem_minfo[getModclsStPrm((pHspCtx), (pInst))->subid])
//# define getModclsStPrm( pHspCtx, pInst ) (&(pHspCtx)->mem_minfo[(pInst)->id_minfo])
//# define getModclsMember( pInst ) ((pInst)->members)
#else
# define getModclsStPrm( pHspCtx, pInst ) (&(pHspCtx)->mem_minfo[getModclsStDat((pHspCtx), (pInst))->prmindex])
# define getModclsStDat( pHspCtx, pInst ) (&(pHspCtx)->mem_finfo[(pInst)->customid])
# define getModclsMember( pInst ) ((pInst)->ptr)
#endif

//##############################################################################
//                定義部 : 互換をとるための定数
//##############################################################################

#ifdef clhsp
# define MODVAR_MAGICCODE MODINST_MAGICCODE
#endif

#endif
