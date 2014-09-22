// 変数関連 utility

#include "mod_varutil.h"
#include "mod_argGetter.h"

namespace hpimod {

//##########################################################
//        HspVarProc 関係
//##########################################################

//##########################################################
//        変数情報の取得
//##########################################################

//------------------------------------------------
// 変数の情報を得る
//------------------------------------------------
int code_varinfo(PVal* pval)
{
	int const varinfo = code_geti();
	switch ( varinfo ) {
		case VarInfo_Flag:   return static_cast<int>(pval->flag);
		case VarInfo_Mode:   return static_cast<int>(pval->mode);
		case VarInfo_Size:   return pval->size;
		case VarInfo_Ptr:    return reinterpret_cast<UINT_PTR>(pval->pt);
		case VarInfo_Master: return reinterpret_cast<UINT_PTR>(pval->master);
		default:
			if ( VarInfo_Len0 <= varinfo && varinfo <= VarInfo_Len4 ) {
				return pval->len[varinfo - VarInfo_Len0];
			}
			puterror(HSPERR_ILLEGAL_FUNCTION);
	}
}



//##########################################################
//        変数の作成
//##########################################################
//------------------------------------------------
// dimtype
//------------------------------------------------
void code_dimtype(PVal* pval, vartype_t vtype)
{
	int len[4] {};

	for ( int i = 0; i < ArrayDimMax; ++i ) {
		len[i] = code_getdi(0);
		if ( len[i] < 0 ) puterror(HSPERR_ILLEGAL_FUNCTION);
		if ( len[i] == 0 ) break;
	}
	exinfo->HspFunc_dim(pval, vtype, 0, len[0], len[1], len[2], len[3]);
	return;
}

//------------------------------------------------
// pval の dimtype 処理
// 
// @ x(a), y(b), z(c) ... による一次元配列の連続初期化。
// @ dim 的用法。
//------------------------------------------------
int code_dimtypeEx( vartype_t vtype, DimFunc_t fDim )
{
	// ひたすらループする
	for ( int i = 0; code_isNextArg(); ++ i ) {

		// 変数のPValポインタと要素数を取得
		PVal* pval;
		APTR const aptr = code_getva( &pval );

		if ( i == 0 && aptr <= 0 ) {	// 先頭で、かつ () なし
			// dimtype 処理
			int idx[4];
			for ( int i = 0; i < 4; ++ i ) {
				idx[i] = code_getdi(0);
			}
			if ( fDim ) {
				(*fDim)(pval, vtype, 0, idx[0], idx[1], idx[2], idx[3]);
			} else {
				exinfo->HspFunc_dim(pval, vtype, 0, idx[0], idx[1], idx[2], idx[3]);
			}
			break;
		}

		if ( fDim ) {
			(*fDim)(pval, vtype, 0, aptr, 0, 0, 0);
		} else {
			exinfo->HspFunc_dim(pval, vtype, 0, aptr, 0, 0, 0);
		}
	}

	return 0;
}

//------------------------------------------------
// 添字から aptr 値を作成する
//------------------------------------------------
APTR CreateAptrFromIndex(PVal const* pval, int idx[4])
{
	int multiple[4];

	multiple[0] = 1;

	// 1 あたり量を算出
	for ( int i = 1; i < 4; ++ i ) {
		multiple[i] = multiple[i - 1] * pval->len[i];
	}

	// 掛け合わせるだけ
	return (
		idx[0] * multiple[0] +
		idx[1] * multiple[1] +
		idx[2] * multiple[2] +
		idx[3] * multiple[3]
	);
}

//------------------------------------------------
// aptr から添字を求める
//------------------------------------------------
void GetIndexFromAptr(PVal const* pval, APTR aptr, int ret[])
{
	APTR tmpAptr = aptr;

	for ( int i = 0; i < ArrayDimMax; ++ i ) ret[i] = 0;

	for ( int i = 0; i < pval->arraycnt; ++ i ) {
		if ( pval->len[i + 1] ) {
			ret[i] = tmpAptr %  pval->len[i + 1];
			         tmpAptr /= pval->len[i + 1];
			if ( tmpAptr == 0 ) break;
		}
	}
	return;
}

} // namespace hpimod
