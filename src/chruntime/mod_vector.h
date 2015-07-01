// module for using object "vector"

#ifndef IG_MODULE_VECTOR_H
#define IG_MODULE_VECTOR_H

#include <vector>

#include "hspvar_core.h"

//##############################################################################
//               型定義
//##############################################################################
//------------------------------------------------
// vector 型の実体
//------------------------------------------------
struct Vector
{
	typedef std::vector<VecElem*> list_t;
	
	list_t* list;
};

struct VecElem
{
	ScrValue* pVal;
};

//------------------------------------------------
// vector 型空間
//------------------------------------------------
NTM_Vt( Vector, Vector*, Vector** );

//##############################################################################
//               関数宣言
//##############################################################################
//**********************************************************
//        準備・初期化・破棄
//**********************************************************
extern void Vector_init ( Vector* self, uint cntElem = 0 );
extern void Vector_init ( Vector* self, const Vector& src, uint idxBgn = 0, int idxEnd = -1 );
extern void Vector_clear( Vector* self );
extern void Vector_term ( Vector* self );

//**********************************************************
//        Vector 操作
//**********************************************************
extern void Vector_cat ( Vector* self, const Vector* src );
extern void Vector_copy( Vector* self, const Vector* src );
extern void Vector_redim( Vector* self, size_t len );

extern VecElem* Vector_at( Vector* self, uint idx );
extern bool Vector_isEqual( const Vector* self, const Vector* src );

//**********************************************************
//        VecElem 操作
//**********************************************************

//##############################################################################
//                GC
//##############################################################################
extern Vector*  new_vector( uint cntElems = 0 );
extern Vector*  new_vector( const Vector* pSrc );
extern Vector*  new_vector_dup  ( const Vector* pSrc );
extern Vector*  new_vector_slice( const Vector* pSrc, uint idxBgn = 0, int idxEnd = -1 );
extern VecElem* new_vecelem( vartype_t vt = Vartype::Int, c_valptr_t p = 0 );
extern VecElem* new_vecelem( ScrValue* pVal );
extern VecElem* new_vecelem( VecElem* elem );
extern VecElem* new_vecelem_dup( VecElem* elem );

extern void vector_register(void);
extern void vecelem_register(void);

//}

#endif
