// 変数関連 utility

#ifndef IG_MODULE_VARIABLE_UTILITY_H
#define IG_MODULE_VARIABLE_UTILITY_H

#include "hsp3plugin_custom.h"

namespace hpimod {

// 変数情報の取得
extern int code_varinfo(PVal* pval);
enum VarInfo {
	VarInfo_Flag = 0,
	VarInfo_Mode,
	VarInfo_Len0,
	VarInfo_Len1,
	VarInfo_Len2,
	VarInfo_Len3,
	VarInfo_Len4,
	VarInfo_Size,
	VarInfo_Ptr,
	VarInfo_Master
};

// 変数の作成
extern void code_dimtype(PVal* pval, vartype_t vtype);
typedef void(*DimFunc_t)(PVal*, vartype_t, int, int, int, int, int);
extern int code_dimtypeEx( vartype_t vt, DimFunc_t fDim = nullptr );		// fDim = nullptr => 通常の dim を用いる

// APTR 関係
extern APTR CreateAptrFromIndex(PVal const* pval, int idx[ArrayDimMax]);
extern void GetIndexFromAptr   (PVal const* pval, APTR aptr, int ret[ArrayDimMax]);

} // namespace hpimod

#endif
