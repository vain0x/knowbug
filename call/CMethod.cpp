// メソッド管理クラス

#include "hsp3plugin_custom.h"

#include <map>
#include <string>

#include "CCaller.h"
#include "CCall.h"
#include "CMethod.h"

//------------------------------------------------
// 構築
//------------------------------------------------
CMethod::CMethod(vartype_t vt)
	: mVartype( vt )
	, mpMethodlist( new methodlist_t )
{ }

//------------------------------------------------
// 解体
//------------------------------------------------
CMethod::~CMethod()
{
	delete [] mpMethodlist; mpMethodlist = nullptr;
	return;
}

//--------------------------------------------
// メソッドを追加
//--------------------------------------------
void CMethod::add( const std::string& name, CFunctor const& functor )
{
	mpMethodlist->insert(
		methodlist_t::value_type( name, functor )
	);
	return;
}

//--------------------------------------------
// メソッドを呼び出す
// @ 命令形式
//--------------------------------------------
void CMethod::call(const std::string& name, PVal* pvThis)
{
	auto iter = mpMethodlist->find( name );

	// メソッドなし
	if ( iter == mpMethodlist->end() ) {
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}

	// 呼び出し
	{
		CCaller caller;
		caller.setFunctor( iter->second );
		caller.addArgByRef( pvThis, pvThis->offset );	// this 引数
		caller.setArgAll();		// 引数の設定
		caller.call();			// 呼び出し
	}

	return;
}
