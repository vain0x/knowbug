// call - command.cpp

#include <stack>
#include <set>

#include "hsp3plugin_custom.h"
#include "mod_func_result.h"

#include "CCall.h"
#include "CCaller.h"

#include "Functor.h"
#include "CBound.h"
#include "CStreamCaller.h"
#include "CLambda.h"
#include "CCoRoutine.h"

#include "cmd_call.h"
#include "cmd_sub.h"

#include "vt_functor.h"

using namespace hpimod;

//------------------------------------------------
// ラベル命令・関数の呼び出し
//------------------------------------------------
int Call( void** ppResult )
{
	vartype_t restype;

	// 呼び出し処理
	{
		CCaller caller;

		// 呼び出し前の処理
		caller.setFunctor();
		caller.setArgAll();

		// 呼び出し
		caller.call();

		// 呼び出し後の処理
		restype = caller.getCallResult( ppResult );
	}

	return restype;
}

//------------------------------------------------
// ラベル命令・関数の仮引数宣言
//------------------------------------------------
void Call_declare()
{
	label_t const lb = code_getlb();
	if ( !lb ) puterror( HSPERR_ILLEGAL_FUNCTION );

	CPrmInfo::prmlist_t&& prmlist = code_get_prmlist();		// 仮引数列

	// 登録
	DeclarePrmInfo( lb, CPrmInfo(&prmlist) );
	return;
}

#if 0
//##########################################################
//        引数情報取得
//##########################################################
//------------------------------------------------
// 呼び出し引数情報を取得する関数
//------------------------------------------------
int Call_arginfo(void** ppResult)
{
	CCall* const pCall = TopCallStack();

	auto const id = static_cast<ARGINFOID>( code_geti() );	// データの種類
	int const iArg = code_getdi(-1);							// 引数の番号

	return SetReffuncResult( ppResult, pCall->getArgInfo(id, iArg) );
}

//------------------------------------------------
// 引数の値を取得する
//------------------------------------------------
int Call_argv(void** ppResult)
{
	CCall* const pCall = TopCallStack();

	int const iArg = code_getdi( 0 );		// 引数の番号

	// 返値を設定する
	PVal* const pvalArg = pCall->getArgv( iArg );
	vartype_t const rettype = pvalArg->flag;		// 返値の型を設定
	*ppResult = PVal_getptr(pvalArg);				// 実体ポインタを求める

	return rettype;
}

//------------------------------------------------
// ラベル関数の返値を設定する
//------------------------------------------------
void Call_retval()
{
	CCall* const pCall = TopCallStack();

	if ( code_getprm() <= PARAM_END ) return;		// 省略可

	vartype_t const vt = mpval->flag;
	void* const pValue = PVal_getptr( mpval );
	pCall->setResult( pValue, vt );
	return;
}

//------------------------------------------------
// ラベル関数の返値となった値を取り出す
// 
// @ call 終了後に呼び出される。
//------------------------------------------------
int Call_result(void** ppResult)
{
	PVal* const pvResult = CCaller::getLastRetVal();

	*ppResult = pvResult->pt;
	return pvResult->flag;
}

//------------------------------------------------
// local 変数の値を取得する
//------------------------------------------------
int Call_getLocal( void** ppResult )
{
	CCall* const pCall = TopCallStack();

	// 前から数えたローカル変数の位置
	int const idxLocal = code_geti();

	PVal* const pvLocal = pCall->getLocal( idxLocal );
	if ( !pvLocal ) puterror( HSPERR_ILLEGAL_FUNCTION );

	*ppResult = pvLocal->pt;
	return pvLocal->flag;
}

//------------------------------------------------
// エイリアスにする
//------------------------------------------------
void Call_alias()
{
	CCall* const pCall = TopCallStack();

	PVal* const pval = code_get_var();
	int const iArg = code_getdi( 0 );

	pCall->alias( pval, iArg );
	return;
}


//------------------------------------------------
// エイリアス名を一括して付ける
//------------------------------------------------
void Call_aliasAll()
{
	CCall* const pCall = TopCallStack();

	// 列挙された変数をエイリアスにする
	for( size_t i = 0
		; code_isNextArg() && ( i < pCall->getCntArg() )
		; ++ i
	) {
		// 変数でなければ無視する
		if ( *type != TYPE_VAR ) {
			code_getprm();

		// 変数を取得
		} else {
			PVal* const pval = code_get_var();
			pCall->alias( pval, i );		// pvalを引数のクローンにする
		}
	}
	return;
}


//------------------------------------------------
// 呼び出されたラベル
//------------------------------------------------
int Call_thislb(void** ppResult)
{
	return SetReffuncResult( ppResult, TopCallStack()->getFunctor()->getLabel() );
}


//##########################################################
//        引数ストリーム呼び出し
//##########################################################
static std::stack<CCaller*> g_stkStream;

//------------------------------------------------
// 引数ストリーム呼び出し::開始
//------------------------------------------------
void Call_StreamBegin()
{
	// 呼び出し前の処理
	g_stkStream.push( new CCaller() );
	CCaller* const pCaller = g_stkStream.top();

	// ラベルの設定
	if ( code_isNextArg() ) {
		Call_StreamLabel();
	}
	return;
}


//------------------------------------------------
// 引数ストリーム呼び出し::ラベル設定
//------------------------------------------------
void Call_StreamLabel()
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );

	CCaller* const pCaller = g_stkStream.top();

	// ジャンプ先の決定
	pCaller->setFunctor();
	return;
}


//------------------------------------------------
// 引数ストリーム呼び出し::追加
//------------------------------------------------
void Call_StreamAdd()
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );

	CCaller* const pCaller = g_stkStream.top();

	// 引数を追加する
	pCaller->setArgAll();

	return;
}


//------------------------------------------------
// 引数ストリーム呼び出し::完了
// 
// @ 命令形式の場合は ppResult == nullptr 。
//------------------------------------------------
int Call_StreamEnd(void** ppResult)
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );

	CCaller* const pCaller = g_stkStream.top();

	// 呼び出し
	pCaller->call();

	// 後処理
	g_stkStream.pop();

	vartype_t const restype = pCaller->getCallResult( ppResult );

	delete pCaller;
	return restype;
}


//------------------------------------------------
// ストリーム呼び出しオブジェクト::生成
//------------------------------------------------
int Call_NewStreamCaller( void** ppResult )
{
	stream_t const stream = CStreamCaller::New();

	// 引数処理
	CCaller* const caller = stream->getCaller();
	{
		caller->setFunctor();
	}

	// functor 型として返却する
	return SetReffuncResult( ppResult, functor_t::make(stream) );
}


//------------------------------------------------
// ストリーム呼び出しオブジェクト::追加
//------------------------------------------------
void Call_StreamCallerAdd()
{
	functor_t&& functor = code_get_functor();
	stream_t const stream = functor->safeCastTo<stream_t>();

	stream->getCaller()->setArgAll();		// 全ての引数を追加する
	return;
}

//##########################################################
//    ユーザ定義命令関係
//##########################################################
//------------------------------------------------
// コマンドを数値化して取得する
//------------------------------------------------
int AxCmdOf(void** ppResult)
{
	int const axcmd = code_get_axcmd();
	if ( axcmd == 0 ) puterror( HSPERR_TYPE_MISMATCH );
	return SetReffuncResult( ppResult, axcmd );
}

//------------------------------------------------
// ユーザ定義命令・関数からラベルを取得する
//------------------------------------------------
int LabelOf(void** ppResult)
{
	int axcmd  = 0;
	label_t lb = nullptr;

	// ユーザ定義コマンド
	if ( *type == TYPE_MODCMD ) {
		axcmd = code_get_axcmd();

	// その他
	} else {
		if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );

		// axcmd
		if ( mpval->flag == HSPVAR_FLAG_INT ) {
			axcmd = VtTraits<int>::derefValptr(mpval->pt);

		// label
		} else if ( mpval->flag == HSPVAR_FLAG_LABEL ) {
			lb = VtTraits<label_t>::derefValptr( mpval->pt );

		// functor
		} else if ( mpval->flag == g_vtFunctor ) {
			lb = FunctorTraits::derefValptr(mpval->pt)->getLabel();
		}
	}

	// ユーザ定義コマンド → ラベル
	if ( !lb ) {
		if ( AxCmd::getType(axcmd) == TYPE_MODCMD ) {
			lb = getLabel( getSTRUCTDAT( AxCmd::getCode(axcmd) )->otindex );

		} else {
			puterror( HSPERR_ILLEGAL_FUNCTION );
		}
	}

	return SetReffuncResult( ppResult, lb );
}

//##########################################################
//    functor 型関係
//##########################################################
//------------------------------------------------
// 型変換関数
//------------------------------------------------
int Functor_cnv(void** ppResult)
{
	HspVarProc* const vp = g_hvpFunctor;

	int prm = code_getprm();
	if ( prm <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );

	// 変換して格納
	*ppResult = vp->Cnv( mpval->pt, mpval->flag );

	return g_vtFunctor;
}

//------------------------------------------------
// 仮引数情報
//------------------------------------------------
int Functor_argc( void** ppResult )
{
	functor_t&& f = code_get_functor();
	return SetReffuncResult( ppResult, static_cast<int>( f->getPrmInfo().cntPrms() ) );
}

int Functor_isFlex( void** ppResult )
{
	functor_t&& f = code_get_functor();
	return SetReffuncResult( ppResult, HspBool(f->getPrmInfo().isFlex()) );
}

//##########################################################
//    その他
//##########################################################
//------------------------------------------------
// コマンドの呼び出し (命令形式)
//------------------------------------------------
void CallCmd_sttm()
{
	int const id = code_geti();

	if ( !(id & AxCmd::MagicCode) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	{
		*type = AxCmd::getType( id );		// 指定したコマンドがあることにする (これにより1つのコード潰れるので、ダミーを配置すべし)
		*val  = AxCmd::getCode( id );
		*exinfo->npexflg = EXFLG_1;
	}
	return;
}

//------------------------------------------------
// コマンドの呼び出し (関数形式)
//------------------------------------------------
int CallCmd_func( void** ppResult )
{
	int const id = code_geti();
	if ( !(id & AxCmd::MagicCode) ) puterror( HSPERR_ILLEGAL_FUNCTION );

	// 関数を呼び出す (その返値を call_func 自体の返値とする)
	{
		*type = AxCmd::getType( id );
		*val  = AxCmd::getCode( id );
		*exinfo->npexflg = 0;

		if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	}

	*ppResult = mpval->pt;
	return mpval->flag;
}

//------------------------------------------------
// 引数束縛
//------------------------------------------------
int ArgBind( void** ppResult )
{
	bound_t const bound = CBound::New();

	// 引数処理
	CCaller* const caller = bound->getCaller();
	{
		caller->setFunctor();	// スクリプトから被束縛関数を取り出す
		caller->setArgAll();	// スクリプトから与えられた引数を全て受け取る (不束縛引数も受け付ける)
	}
	bound->bind();

	// functor 型として返却する
	return SetReffuncResult( ppResult, functor_t::make(bound) );
}

//------------------------------------------------
// 束縛解除
//------------------------------------------------
int UnBind( void** ppResult )
{
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	if ( mpval->flag != g_vtFunctor ) puterror( HSPERR_TYPE_MISMATCH );

	auto const functor = FunctorTraits::derefValptr(mpval->pt);
	auto const bound   = functor->safeCastTo<bound_t>();

	return SetReffuncResult( ppResult, bound->unbind() );
}

//------------------------------------------------
// 束縛関数の一括解放
//------------------------------------------------
void ReleaseBounds()
{
	g_resFunctor.clear();
	return;
}

//------------------------------------------------
// ラムダ式
// 
// @ funcexpr(args...) → function() { lambdaBody args... : return }
//------------------------------------------------
int Call_Lambda( void** ppResult )
{
	auto const lambda = CLambda::New();

	lambda->code_get();

	return SetReffuncResult( ppResult, functor_t::make(lambda) );
}

//------------------------------------------------
// lambda が内部で用いるコマンド
// 
// @ LambdaBody:
// @	引数を順に取り出し、それぞれを local 変数に代入していく。
// @	最後の引数は返値として受け取る。
// @	ゆえに、この命令の引数は必ず (local 変数の数 + 1) 存在する。
// @ LambdaValue:
// @	idx 番目の local 変数を取り出す。
//------------------------------------------------
void Call_LambdaBody()
{
	CCall* const pCall     = TopCallStack();
	size_t const cntLocals = pCall->getPrmInfo().cntLocals();

	for ( size_t i = 0; i < cntLocals; ++ i ) {
		code_getprm();
		PVal* const pvLocal = pCall->getLocal( i );
		PVal_assign( pvLocal, mpval->pt, mpval->flag );
	}

	Call_retval();
	return;
}

//------------------------------------------------
// コルーチン::生成
//------------------------------------------------
int Call_CoCreate( void** ppResult )
{
	auto coroutine = CCoRoutine::New();

	CCaller* const caller = coroutine->getCaller();
	caller->setFunctor();		// functor を受ける
	caller->setArgAll();

	return SetReffuncResult( ppResult, functor_t::make(coroutine) );
}

//------------------------------------------------
// コルーチン::中断実装
//------------------------------------------------
void Call_CoYieldImpl()
{
	Call_retval();		// 返値を受け取る

	// newlab される変数をコルーチンに渡す
	PVal* const pvNextLab = code_get_var();
	CCoRoutine::setNextVar( pvNextLab );	// static 変数に格納する

	return;
}
#endif

//------------------------------------------------
// 終了処理
//------------------------------------------------
void Call_Term()
{
	CCaller::releaseLastRetVal();
	return;
}

//##########################################################
//    テストコード
//##########################################################
#ifdef _DEBUG

void CallHpi_test(void)
{
	//

	return;
}

#endif
