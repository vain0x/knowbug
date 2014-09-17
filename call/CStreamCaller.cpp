// ストリーム呼び出しクラス
#if 0
#include "CStreamCaller.h"

#include "CCall.h"
#include "CCaller.h"

#include "mod_makepval.h"

using namespace hpimod;

static void AddAllArgsByRef( CCaller& caller, CCall const& callStream );

//------------------------------------------------
// 構築 (ラッパー)
//------------------------------------------------
stream_t CStreamCaller::New()
{
	return new CStreamCaller();
}

//------------------------------------------------
// 構築
//------------------------------------------------
CStreamCaller::CStreamCaller()
	: IFunctor()
	, mpCaller( new CCaller )
{ }

//------------------------------------------------
// 破棄
//------------------------------------------------
CStreamCaller::~CStreamCaller()
{
	delete mpCaller; mpCaller = nullptr;
	return;
}

//------------------------------------------------
// 呼び出し処理
// 
// @ 引数なし => ストリームの引数(の参照)だけを用いて呼び出す
// @ 引数あり => ストリームの引数と与引数の連結を作って呼び出す
//------------------------------------------------
void CStreamCaller::call( CCaller& callerGiven )
{
	CCall& callStream = mpCaller->getCall();
	CCall& callGiven  = callerGiven.getCall();

	// 引数なし : 簡易呼び出し (callerGiven を使いまわせる)
	if ( callGiven.getCntArg() == 0 ) {
		callerGiven.setFunctor( callStream.getFunctor() );		// ストリームが呼んでいる関数

		AddAllArgsByRef( callerGiven, callStream );

		// 呼び出す
		callerGiven.call();

	// 引数あり
	} else {
		CCaller caller;

		caller.setFunctor( callStream.getFunctor() );

		// 引数列 : ストリームの引数と与引数を並べる
		{
			AddAllArgsByRef( caller, callStream );
			AddAllArgsByRef( caller, callerGiven.getCall() );
		}

		// 呼び出す
		caller.call();

		// 返値を callerGiven に転送する
		callGiven.setRetValTransmit( caller.getCall() );
	}

	return;
}

//------------------------------------------------
// ストリームが持つ引数を caller に追加する
//------------------------------------------------
static void AddAllArgsByRef( CCaller& caller, CCall const& callStream )
{
	// ストリームに与えられていた実引数をそのまま引き渡す
	for ( size_t i = 0; i < callStream.getCntArg(); ++ i ) {
		caller.addArgByRef(
			callStream.getArgPVal(i),
			callStream.getArgAptr(i)
		);
	}
	return;
}

#if 0
	/* ボツ
		// 与えられた引数を全てストリームに追加し、取り除く
		for ( int i = 0; i < callNow->getCntArg(); ++ i ) {
			PVal* const pval = callNow->getArgPVal(i);
			APTR  const aptr = callNow->getArgAptr(i);

			if ( callNow->getArgInfo( ARGINFOID_BYREF, i ) ) {	// 参照渡し
				mpCaller->addArgByRef( pval, aptr );
			} else {
				mpCaller->addArgByVal( PVal_getptr(pval, aptr), pval->flag );
			}
		}
		callNow->clearArg();
	//*/
#endif

//------------------------------------------------
// 取得系
//------------------------------------------------
label_t         CStreamCaller::getLabel()   const { return getCaller()->getCall().getFunctor().getLabel(); }
int             CStreamCaller::getAxCmd()   const { return getCaller()->getCall().getFunctor().getAxCmd(); }
CPrmInfo const& CStreamCaller::getPrmInfo() const { return getCaller()->getCall().getFunctor().getPrmInfo(); }
int             CStreamCaller::getUsing()   const { return 1; }

//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// 
//------------------------------------------------
#endif