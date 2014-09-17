// Call(Method) - Command
#if 0
#include <vector>
#include <map>
#include <string>

#include "cmd_method.h"
#include "cmd_sub.h"

#include "CCaller.h"
#include "CCall.h"
#include "CPrmInfo.h"

using methods_t = std::map<std::string, functor_t>;
static std::map<vartype_t, methods_t> g_distribute;

static void ObjectMethodCustom(PVal* pval);

//------------------------------------------------
// メソッド呼び出し関数のすり替え
// 
// @prm p1 = vt : 型タイプ値
//------------------------------------------------
static void Method_replaceProc(vartype_t vtype)
{
	HspVarProc* const vp = getHvp( vtype );

	// メンバの持つ関数ポインタを書き換える
	vp->ObjectMethod = ObjectMethodCustom;

	// 空のメソッドクラスを作り、登録しておく
	auto const iter = g_distribute.find(vtype);
	if ( iter == g_distribute.end() ) {
		g_distribute.insert({ vtype, methods_t {} });

	} else {
		dbgout("型 %s のメソッドは既にすり替えられている。", vp->vartype_name);
		puterror(HSPERR_ILLEGAL_FUNCTION);
	}
	return;
}

void Method_replace()
{
	int const vt = code_get_vartype();
	Method_replaceProc(vt);
	return;
}

//------------------------------------------------
// メソッドの追加
// 
// @prm p1 = vt  : 型タイプ値
// @prm p2 = str : メソッド名称 (or default)
// @prm p3 = def : 定義 (ラベル + 仮引数リスト, axcmd)
//------------------------------------------------
void Method_add()
{
	vartype_t const vtype = code_get_vartype();
	std::string const name = code_gets();

	// 呼び出し先 or ラベル関数宣言の取得
	functor_t&& functor = code_get_functor();

	// ラベル => 仮引数リストを受け取る
	if ( functor.getType() == FuncType_Label ) {
		CPrmInfo::prmlist_t&& prmlist = code_get_prmlist();
		prmlist.insert( prmlist.begin(), PRM_TYPE_VAR );		// 先頭に var this を追加
		DeclarePrmInfo( functor.getLabel(), std::move(CPrmInfo(&prmlist)) );
	}

	// CMethod に追加
	auto const iter = g_distribute.find(vtype);
	if ( iter != g_distribute.end() ) {
		auto& methods = iter->second;

		methods.insert({ name, functor });

	} else {
		dbgout("Method_replace されていません！");
		puterror(HSPERR_UNSUPPORTED_FUNCTION);
	}
	return;
}

//------------------------------------------------
// メソッド呼び出し元の変数のクローンを作る
//------------------------------------------------
void Method_cloneThis()
{
	CCall* const pCall     = TopCallStack();
	PVal*  const pvalClone = code_getpval();

	PVal_clone( pvalClone, pCall->getArgPVal(0), pCall->getArgAptr(0) );
	return;
}

//##############################################################################
//                method 下請け
//##############################################################################
//------------------------------------------------
// メソッド呼び出し関数 ( method.hpi 版 )
//------------------------------------------------
static void ObjectMethodCustom(PVal* pval)
{
	vartype_t const vtype = pval->flag;
	std::string const name = code_gets();

	auto const iter = g_distribute.find(vtype);
	if ( iter == g_distribute.end() ) {
		auto& methods = iter->second;
		
		auto const iter = methods.find(name);
		if ( iter != methods.end() ) {
			auto& functor = iter->second;

			dbgout("未実装");
#if 0
			// 呼び出し
			{
				CCaller caller;
				caller.setFunctor(functor);

				// this 引数を追加する
				caller.addArgByRef(pvThis, pvThis->offset);

				caller.setArgAll();
				caller.call();
			}
#endif
		} else {
			puterror(HSPERR_UNSUPPORTED_FUNCTION);
		}

	} else {
		// Method_replace していない型のメソッド
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return;
}
#endif
