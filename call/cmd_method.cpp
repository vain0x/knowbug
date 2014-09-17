// Call(Method) - Command

#include <vector>
#include <map>
#include <string>

#include "cmd_method.h"
#include "cmd_sub.h"

#include "CCaller.h"
#include "CCall.h"
#include "CPrmInfo.h"

#include "CMethod.h"
#include "CMethodlist.h"

//################################################
//    グローバル変数
//################################################
static auto g_pMethodlist = (new CMethodlist);

//################################################
//    グローバル関数
//################################################
static void ObjectMethodCustom(PVal* pval);

static void Method_replace_proc(int vt);

//------------------------------------------------
// メソッド呼び出し関数のすり替え
// 
// @prm p1 = vt : 型タイプ値
//------------------------------------------------
void Method_replace(void)
{
	int const vt = code_get_vartype();
	Method_replace_proc(vt);
	return;
}

static void Method_replace_proc(int vt)
{
	HspVarProc* const vp = getHvp( vt );

	// メンバの持つ関数ポインタを書き換える
	vp->ObjectMethod = ObjectMethodCustom;

	// 空のメソッドクラスを作り、登録しておく
	g_pMethodlist->set( vt );
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

#if 0
	// ラベル => 仮引数リストを受け取る
	if ( functor.getType() == FuncType_Label ) {
		CPrmInfo::prmlist_t&& prmlist = code_get_prmlist();
		prmlist.insert( prmlist.begin(), PRM_TYPE_VAR );		// 先頭に var this を追加
		DeclarePrmInfo( functor.getLabel(), std::move(CPrmInfo(&prmlist)) );
	}
#endif

	// CMethod に追加
	CMethod* const pMethod = g_pMethodlist->get( vtype );

	if ( pMethod ) {
		pMethod->add( name, functor );

	} else {
		// エラータイプが変
		dbgout("Method_replace されていません！");
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
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
	int const vt = pval->flag;
	std::string const name = code_gets();

	CMethod* const pMethod = g_pMethodlist->get( vt );

	if ( pMethod ) {
		pMethod->call( name, pval );
	} else {
		// Method_replace していない型のメソッド
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	return;
}
