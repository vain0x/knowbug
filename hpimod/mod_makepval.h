// PVal 構造体の独自管理

#ifndef IG_MODULE_HPIMOD_MAKE_PVAL_H
#define IG_MODULE_HPIMOD_MAKE_PVAL_H

#include "hsp3plugin_custom.h"

namespace hpimod {

//##########################################################
//        関数宣言
//##########################################################
extern void PVal_init( PVal* pval, vartype_t vtype );
extern void PVal_alloc( PVal* pval, PVal* pval2 = nullptr, vartype_t vtype = HSPVAR_FLAG_NONE );
extern void PVal_clear( PVal* pval, vartype_t vtype );
extern void PVal_free( PVal* pval );

extern PVal* PVal_getDefault( vartype_t vt = HSPVAR_FLAG_INT );

// 取得系
//extern size_t PVal_cntElems( PVal const* pval );
extern size_t PVal_size( PVal const* pval );
extern PDAT*  PVal_getptr( PVal* pval );
extern PDAT*  PVal_getptr( PVal* pval, APTR aptr );

// 操作系
extern void PVal_assign( PVal* pvLeft, PDAT const* data, vartype_t vtype );
extern void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs );
extern void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, PVal* pvTmp );
extern void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs );
extern void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs, PVal* pvTmp );
extern void PVal_copy( PVal* pvDst, PVal* pvSrc );
extern void PVal_swap( PVal* pvLhs, PVal* pvRhs );
extern void PVal_swap( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs );
extern void PVal_clone( PVal* pvDst, PVal* pvSrc, APTR aptrSrc = -1 );
extern void PVal_clone( PVal* pvDst, PDAT* ptr, int flag, int size );
extern void PVal_cloneVar( PVal* pvDst, PVal* pvSrc, APTR aptrSrc = -1 );

// その他
extern void SetResultSysvar( PDAT const* data, vartype_t vtype );
extern PDAT const* Valptr_cnvTo( PDAT const* data, vartype_t vtSrc, vartype_t vtDst );

extern int PVal_supportMeta( PVal* pval );
extern int PVal_supportNotmeta( PVal* pval );

extern bool PVal_supportArray( PVal* pval );

// OpenHSP からの引用
//void HspVarCoreDupptr( PVal* pval, int flag, void* ptr, int size );
//void HspVarCoreDup( PVal* pval, PVal* arg, APTR aptr );

} // namespace hpimod

#endif
