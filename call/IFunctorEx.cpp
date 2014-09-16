// 特殊関数子インターフェース
// 既定動作の定義

#include "IFunctorEx.h"
#include <vector>

static std::vector<exfunctor_t> g_allExFunctors;

//------------------------------------------------
// 構築
//------------------------------------------------
IFunctorEx::IFunctorEx()
	: mRefer( 0 )
{
#ifdef DBGOUT_FUNCTOR_EX_ADDREF_OR_RELEASE
	static int stt_counter = 0; mId = ++ stt_counter;
#endif
//	dbgout( "new!" );
//	g_allExFunctors.push_back( this ); AddRef();	// 全実体コンテナに登録

	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
IFunctorEx::~IFunctorEx()
{
	// 全実体コンテナから登録解除
	/*
	auto iter = std::find( g_allExFunctors.begin(), g_allExFunctors.end(), this );
	if ( iter != g_allExFunctors.end() ) {
		*iter = nullptr;
		dbgout( "unregister!" );
	}
	//*/

//	dbgout( "delete!" );
}

void IFunctorEx::Delete( exfunctor_t& self )
{
	delete self; self = nullptr;
	return;
}

void IFunctorEx::ReleaseAllInstance()
{
	/*
	// 全実体コンテナの解放
	for each ( auto iter in g_allExFunctors ) {
		FunctorEx_Release( iter );
	}

	g_allExFunctors.clear();
	//*/
	return;
}

