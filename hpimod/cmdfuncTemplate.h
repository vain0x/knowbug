//template functions for plugin command dispatcher

#ifndef IG_HPIMOD_CMDFUNC_TEMPLATE_H
#define IG_HPIMOD_CMDFUNC_TEMPLATE_H

#include "./hsp3plugin_custom.h"

namespace hpimod
{

static PVal* code_get_var()
{
	PVal* pval;
	code_getva(&pval);
	return pval;
}

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
template< int(*ProcFunc  )(int, PDAT**),
          int(*ProcSysvar)(int, PDAT**) >
static void* reffunc(int* type_res, int cmd)
{
	PDAT* pResult = nullptr;

	if ( !(*type == TYPE_MARK && *val == '(') ) {
		*type_res = ProcSysvar(cmd, &pResult);

	} else {
		code_next();

		*type_res = ProcFunc(cmd, &pResult);

		if ( !(*type == TYPE_MARK && *val == ')') ) puterror(HSPERR_INVALID_FUNCPARAM);
		code_next();
	}

	if ( !pResult ) puterror(HSPERR_NORETVAL);
	return pResult;
}

} // namespace hpimod

#endif
