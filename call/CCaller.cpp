// CCaller

#include <map>

#include "iface_call.h"		// plugin-type, CallCmdId が欲しいので
#include "cmd_sub.h"

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"
#include "mod_makepval.h"

#include "CCaller.h"
#include "CCall.h"
#include "CPrmInfo.h"

#include "CFunctor.h"
#include "CBound.h"

using namespace hpimod;

static std::stack<CCall*> g_stkCall;

//------------------------------------------------
// 構築
// 
// @ 呼び出し開始前
//------------------------------------------------
CCaller::CCaller()
	: CCaller( CallMode::Proc )
{ }

CCaller::CCaller( CallMode mode )
	: mpCall( new CCall )
	, mMode ( mode )
{ }

//------------------------------------------------
// call
//------------------------------------------------
void CCaller::call(void)
{
	// 足りない引数を補う
	mpCall->completeArg();

	// 独自のCallスタックに積む
	g_stkCall.push( mpCall.get() );

	// 呼び出し
	mpCall->call( *this );

	// 独自のCallスタックから降ろす
	if ( g_stkCall.empty() ) puterror(HSPERR_STACK_OVERFLOW);
	g_stkCall.pop();
	return;
}

//------------------------------------------------
// ジャンプ先を設定する
// 
// @ 中間コードの次から得る
//------------------------------------------------
void CCaller::setFunctor()
{
	CFunctor&& functor = code_get_functor();
	setFunctor( functor );
	return;
}

//------------------------------------------------
// ジャンプ先を設定する
// 
// @ CFunctor を用いる
//------------------------------------------------
void CCaller::setFunctor( CFunctor const& functor )
{
	mpCall->setFunctor( functor );
	return;
}

//------------------------------------------------
// すべての引数を取り出して追加する
//------------------------------------------------
void CCaller::setArgAll()
{
	// 引数をすべて取り出す
	while ( setArgNext() )
		;
	return;
}

//------------------------------------------------
// 次の引数を一つ取得し、追加する [中間コード]
// 
// @result: 引数を取り出したか？
//------------------------------------------------
bool CCaller::setArgNext()
{
	if ( !code_isNextArg() ) return false;	// 引数が続かない (次が文頭 or ')' の) 場合

	// 次の引数を1つ取得する
	int const prmtype = mpCall->getPrmType( mpCall->getCntArg() );

	// 不束縛引数 (nobind)
	if ( *type == g_pluginType_call && *val == CallCmdId::NoBind ) {
		if ( mMode != CallMode::Bind ) puterror( HSPERR_ILLEGAL_FUNCTION );
		code_next();

		int priority = 0;		// (既定: 0)

		if ( *type == TYPE_MARK && *val == '(' ) {
			// () あり
			code_next();

			priority = code_getdi( priority );	// 省略可

			if ( !code_next_expect( TYPE_MARK, ')') ) puterror( HSPERR_TOO_MANY_PARAMETERS );

		} else {
			// () なし => 引数の取り出しの完了処理をする
			if ( *exinfo->npexflg & EXFLG_2 ) {
				*exinfo->npexflg &= ~EXFLG_2;	// 引数を取り出した

			// nobind の後に許容できない何かがある
			} else if ( code_isNextArg() ) {
				puterror( HSPERR_SYNTAX );	
			}
		}

		mpCall->addArgSkip( std::max(priority + 0xFF00, 0) );			// バイアスをかけて正数にする

	// 参照渡し (明示: byref or bythismod)
	} else if ( *type == g_pluginType_call && (*val == CallCmdId::ByRef || *val == CallCmdId::ByThismod) ) {
		code_next();

		// [ call_byref || var ]
		PVal* const pval = code_get_var();

		if ( !code_next_expect( TYPE_MARK, CALCCODE_OR ) ) puterror( HSPERR_SYNTAX );

		mpCall->addArgByRef( pval );

	// 参照渡し (仮引数に基づく)
	} else if ( PrmType_IsRef(prmtype) ) {
		PVal* pval;

		switch ( prmtype ) {
			case PRM_TYPE_ARRAY:  pval = code_getpval(); break;
			case PRM_TYPE_VAR:    //
			case PRM_TYPE_MODVAR: pval = code_get_var(); break;
			default:
				puterror( HSPERR_SYNTAX );
		}

		mpCall->addArgByRef( pval );

	// 値渡し
	} else {
		int const iArg = mpCall->getCntArg();

		int prm;

		// 引数省略 (明示)
		if ( *type == g_pluginType_call && *val == CallCmdId::ByDef ) {
			code_next();
			prm = PARAM_DEFAULT;

		} else {
			// 次の引数
			prm = code_getprm();
			if ( prm == PARAM_END || prm == PARAM_ENDSPLIT ) return false;
		}

		{
			PVal* const pval = ( prm == PARAM_DEFAULT )
				? ( mpCall->getDefaultArg( iArg ) )				// 省略時
				: mpval;

			mpCall->addArgByVal( pval );
		}

		// 仮引数に従って型チェック
		{
			PVal* const pval = mpCall->getArgPVal(iArg);		// mpCall に追加された変数

			//*
			// (応急処置) double ← int を許可 : 後に修正すべき
			if ( mpCall->getPrmInfo().getPrmType( iArg ) == HSPVAR_FLAG_DOUBLE
				&& pval->flag == HSPVAR_FLAG_INT
			) {
				double value = *(int*)pval->pt;
				code_setva( pval, 0, HSPVAR_FLAG_DOUBLE, &value );
			}
			//*/

			mpCall->checkCorrectArg( pval, iArg );		// mpCall に追加する前にした方が良い
		}
	}

	return true;
}

//------------------------------------------------
// 値渡し引数を追加する
// 
// @ 型チェックなどしないので注意
//------------------------------------------------
void CCaller::addArgByVal( void const* val, vartype_t vt )
{
	return mpCall->addArgByVal( val, vt );
}

//------------------------------------------------
// 参照渡し引数を追加する
// 
// @ 型チェックなどしないので注意
//------------------------------------------------
void CCaller::addArgByRef( PVal* pval )
{
	return addArgByRef( pval, pval->offset );
}

void CCaller::addArgByRef( PVal* pval, APTR aptr )
{
	return mpCall->addArgByRef( pval, aptr );
}

//------------------------------------------------
// 変数複製引数を追加する
//------------------------------------------------
void CCaller::addArgByVarCopy( PVal* pval )
{
	return mpCall->addArgByVarCopy( pval );
}

//##########################################################
//    取得
//##########################################################
//------------------------------------------------
// 返値を取得する
// 
// @ 呼び出し終了の直後にのみ使える。
// @ 受け取った返値のポインタはすぐに死亡する危険がある。
// @ 返値がなければ nullptr が返る。
//------------------------------------------------
PVal* CCaller::getRetVal() const
{
	return ( ctx->retval_level == (ctx->sublev + 1) )
		? *exinfo->mpval			// return に設定されている場合、それを優先する
		: mpCall->getRetVal();		// call_retval によって与えられた返値
}

//------------------------------------------------
// call の返値を取得する
// 
// @ 静的変数に複写(バックアップ)してから得る。
// @ 他の caller が呼び出しを終了するまで死なない。
// @prm ppResult : 返値への実体ポインタを格納する void*  型の変数へのポインタ
// @result       : 返値の型タイプ値
//------------------------------------------------
vartype_t CCaller::getCallResult(void** ppResult)
{
	if ( !ppResult ) return HSPVAR_FLAG_NONE;
	*ppResult = nullptr;

	PVal* const pvResult = getRetVal();
	if ( !pvResult ) return HSPVAR_FLAG_NONE;

	// 最初の呼び出しの場合、stt_respval を初期化する
	if ( !stt_respval ) {
		stt_respval = reinterpret_cast<PVal*>( hspmalloc(sizeof(PVal)) );
		PVal_init( stt_respval, pvResult->flag );
	}

	// 静的変数に値を保存しておく
	code_setva(
		stt_respval, 0, pvResult->flag,
		PVal_getptr(pvResult)
	);

	*ppResult = PVal_getptr( stt_respval );
	return stt_respval->flag;
}

//------------------------------------------------
// 直前の call の返値を返す
// @static
//------------------------------------------------
PVal* CCaller::getLastRetVal()
{
	return stt_respval;
}

void CCaller::releaseLastRetVal()
{
	if ( stt_respval ) {
		PVal_free( stt_respval );
		hspfree( stt_respval );
		stt_respval = nullptr;
	}
	return;
}

//##########################################################
//    グローバル変数の初期化
//##########################################################
PVal* CCaller::stt_respval = nullptr;

//##########################################################
//    下請け関数
//##########################################################
//------------------------------------------------
// 引数スタックのトップを返す
// 
// @ スタックが空なら nullptr を返す。
//------------------------------------------------
CCall* TopCallStack()
{
	if ( g_stkCall.empty() ) return nullptr;

	return g_stkCall.top();
}
