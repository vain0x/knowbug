// vector - VarProc header

#ifndef IG_VECTOR_VARPROC_H
#define IG_VECTOR_VARPROC_H

#include <vector>
#include "vp_template.h"
#include "Managed.h"
#include "ManagedPVal.h"
#include "HspAllocator.h"

//#include "CVector.h"

using namespace hpimod;

using vector_t = Managed< std::vector<ManagedPVal, HspAllocator<ManagedPVal>>, false >;

// vartype tag
struct vector_tag
{
	// interfaces
	using value_t  = vector_t;
	using valptr_t = value_t*;
	using const_valptr_t = value_t const*;

	using master_t = value_t;	// 実体値

	static int const basesize = sizeof(value_t);

	// special indexes
	static int const IdxBegin = 0;
	static int const IdxLast  = (-0x031EC10A);	// 最後の要素を表す添字コード
	static int const IdxEnd   = (-0x031EC10B);	// (最後の要素 + 1)を表す添字コード
};

// VtTraits<> の特殊化
namespace hpimod
{
	template<> struct VtTraits<vector_tag> : public VtTraitsBase<vector_tag>
	{
		//------------------------------------------------
		// 内部変数の取得
		// 
		// @ 本体が参照されているときは nullptr を返す。
		//------------------------------------------------
		static PVal* getInnerPVal(PVal* pval, APTR aptr)
		{
			return getMaster(pval)->at(aptr).valuePtr();
		}

		static PVal* getInnerPVal(PVal* pval)
		{
			if ( pval->arraycnt == 0 ) return nullptr;
			return getInnerPVal(pval, pval->offset);
		}

	};
}

using VectorTraits = hpimod::VtTraits<vector_tag>;

// グローバル変数
extern vartype_t g_vtVector;
extern HspVarProc* g_pHvpVector;

// 関数
extern void  HspVarVector_Init( HspVarProc* p );
extern int SetReffuncResult( void** ppResult, vector_t self );

extern void* Vector_indexRhs( vector_t self, int* mptype );

#endif
