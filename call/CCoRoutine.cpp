// コルーチンクラス

#include "CCoRoutine.h"

#include "CCall.h"
#include "CCaller.h"
#include "CFunctor.h"
#include "CPrmInfo.h"

using namespace hpimod;

PVal const* CCoRoutine::stt_pvNextVar = nullptr;

//------------------------------------------------
// 構築 (ラッパー)
//------------------------------------------------
coroutine_t CCoRoutine::New()
{
	return new CCoRoutine();
}

//------------------------------------------------
// 構築
//------------------------------------------------
CCoRoutine::CCoRoutine()
	: mpCaller( new CCaller )
{ }

//------------------------------------------------
// 破棄
//------------------------------------------------
CCoRoutine::~CCoRoutine()
{
	delete mpCaller; mpCaller = nullptr;
	return;
}

//------------------------------------------------
// 呼び出し処理
// 
// @ 関数を呼び出す or 実行を再開する。
//------------------------------------------------
void CCoRoutine::call( CCaller& callerGiven )
{
	mpCallerGiven = &callerGiven;

	{
		// 呼び出す
		mpCaller->call();

		// 次の呼び出し先を再設定する
		if ( stt_pvNextVar ) {
			if ( stt_pvNextVar->flag != HSPVAR_FLAG_LABEL ) puterror( HSPERR_TYPE_MISMATCH );
			label_t const lb = *(label_t*)stt_pvNextVar->pt;

			mpCaller->setFunctor( lb );		// 次の呼び出し先を確定
			stt_pvNextVar = nullptr;
		}

		// 返値を callerGiven に転送する
		callerGiven.getCall().setRetValTransmit( mpCaller->getCall() );
	}
	return;
}

//------------------------------------------------
// 仮引数
//------------------------------------------------
CPrmInfo const& CCoRoutine::getPrmInfo() const
{
	return CPrmInfo::noprmFunc;
}

//------------------------------------------------
// 
//------------------------------------------------

