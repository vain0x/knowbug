// hsp3plugin 拡張ヘッダ (for ue_dai)

#ifndef IG_HSP3PLUGIN_CUSTOM_H
#define IG_HSP3PLUGIN_CUSTOM_H

#include <windows.h>
#undef max
#undef min

#include "../hspsdk/hsp3plugin.h"
#undef stat	// いくつかの標準ライブラリと衝突してしまうので

#include "./basis.h"

namespace hpimod
{

// var 引数を取り出す (code_getva() と同じだが aptr 値ではなく PVal* を返す)
static PVal* code_get_var()
{
	PVal* pval;
	code_getva(&pval);
	return pval;
}

// プラグイン・インターフェース用の関数テンプレート
//------------------------------------------------
// 命令コマンド呼び出し関数
//------------------------------------------------
template<int(*ProcSttmCmd)(int)>
static int cmdfunc(int cmd)
{
	code_next();
	return ProcSttmCmd(cmd);
}

//------------------------------------------------
// 関数コマンド呼び出し関数
//------------------------------------------------
template< int(*ProcFunc  )(int, void**),
          int(*ProcSysvar)(int, void**) >
static void* reffunc( int* type_res, int cmd )
{
	void* pResult = nullptr;

	if ( !(*type == TYPE_MARK && *val == '(') ) {

		*type_res = ProcSysvar( cmd, &pResult );

	} else {
	//	if ( !(*type == TYPE_MARK && *val == '(') ) puterror( HSPERR_INVALID_FUNCPARAM );
		code_next();

		*type_res = ProcFunc( cmd, &pResult );	// コマンド分岐

		if ( !(*type == TYPE_MARK && *val == ')') ) puterror( HSPERR_INVALID_FUNCPARAM );
		code_next();
	}

	if ( !pResult ) puterror( HSPERR_NORETVAL );
	return pResult;
}

} // namespace hpimod

#endif
