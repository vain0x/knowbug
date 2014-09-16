// assoc - VarProc header

#ifndef IG_ASSOC_VARPROC_H
#define IG_ASSOC_VARPROC_H

#include "hsp3plugin_custom.h"

class CAssoc;

// グローバル変数の宣言
extern short g_vtAssoc;
extern HspVarProc* g_pHvpAssoc;

// 関数の宣言
extern void HspVarAssoc_Init( HspVarProc* );

extern CAssoc* code_get_assoc();
extern PVal*   code_get_assoc_pval();

// 定数
static int const assocIndexFullslice = 0xFABC0000;

// マクロ

// 定義
namespace VtAssoc
{
	using value_t = CAssoc*;
	using valptr_t = value_t*;
	using const_valptr_t = value_t const*;
	using master_t = PVal*;
	size_t const basesize = sizeof(value_t);

	static valptr_t asValptr(void* pdat) { return reinterpret_cast<valptr_t>(pdat); }
	static const_valptr_t asValptr(void const* pdat) { return reinterpret_cast<const_valptr_t>(pdat); }
	static valptr_t getPtr(PVal* pval) { return (reinterpret_cast<value_t*>(pval->pt) + pval->offset); }
	static master_t getMaster(PVal* pval) { return reinterpret_cast<master_t>(pval->master); }
}

#endif
