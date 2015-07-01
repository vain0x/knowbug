// chruntime module - for StArray

#ifndef IG_MODULE_ST_ARRAY_H
#define IG_MODULE_ST_ARRAY_H

#include "hspvar_core.h"

//##############################################################################
//               型定義
//##############################################################################
//------------------------------------------------
// array 型の実体
//------------------------------------------------
struct StArray
{
	vartype_t vt;
	valmode_t mode;
	uint len[ArrayDimMax];
	void* ptr;					// 実体の配列が格納される
};

//------------------------------------------------
// array 型空間 (VtArray)
//------------------------------------------------
NTM_Vt( Array, StArray*, StArray** )

//##############################################################################
//               関数宣言
//##############################################################################

//**********************************************************
//        StArray 操作
//**********************************************************
extern void StArray_cat ( StArray* self, const StArray* src );
extern void StArray_copy( StArray* self, const StArray* src );
extern void StArray_redim( StArray* self, uint len );

extern valptr_t StArray_getptr( const StArray* self, arridx_t idx, const VarProc* vp = NULL );
extern arridx_t StArray_getArrIdx( const StArray* self, uint idx1 = 0, uint idx2 = 0, uint idx3 = 0 );

extern uint StArray_cntElems( const StArray* self );
extern bool StArray_isEqual( const StArray* self, const StArray* starrSrc );

//**********************************************************
//        VecElem 操作
//**********************************************************

//##############################################################################
//                GC
//##############################################################################
extern void array_register(void);

extern StArray* new_array( vartype_t vt, valmode_t mode, void* ptr = 0, uint len1 = 0, uint len2 = 0, uint len3 = 0 );
extern StArray* new_array( vartype_t vt, uint len1 = 0, uint len2 = 0, uint len3 = 0 );

extern StArray* new_array_dup( const StArray* src );
extern StArray* new_array_clone( vartype_t vt, bufptr_t pBuf, uint len1, uint len2 = 0, uint len3 = 0  );

#endif
