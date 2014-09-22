// reference - VarProc header

#ifndef IG_REFERENCE_VARPROC_H
#define IG_REFERENCE_VARPROC_H

#include "hsp3plugin_custom.h"

// グローバル変数の宣言
extern short g_vtReference;
extern HspVarProc* g_pHvpReference;

// 関数の宣言
extern void HspVarReference_Init( HspVarProc* );

extern PVal* code_get_reference();

// 定数

// マクロ

// 定義
namespace VtReference
{
	struct PValAfterMaster {		// PVal::master 以降のメンバ
		void	*master;			// 参照する PVal* として使う
		unsigned short	support;
		short	arraycnt;
		int		offset;
		int		arraymul;			// 参照する変数に紐付いた APTR として使う (連想配列サポートの場合ここは書き換わらないみたいなので)
	};
	
	typedef PValAfterMaster value_t;
	typedef value_t* valptr_t;		// ⇔ PDAT*
	const int basesize = sizeof(value_t);
	const int IdxDummy = 0x11235813;
	
	extern valptr_t GetPtr( PVal* pval );
}

#endif
