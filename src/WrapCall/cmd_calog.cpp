// call - command.cpp

#include "hsp3plugin_custom.h"
#include "mod_varutil.h"
#include "mod_utility.h"

#include "CCall.h"
#include "CCaller.h"

#include "cmd_call.h"
#include "cmd_sub.h"

#include "vt_caller.h"

//##########################################################
//    グローバル変数
//##########################################################
static std::stack<CCaller *> g_stkStream;

// 返値用
static int g_resIntVal     = 0;
static label_t g_resLblVal = NULL;

//##########################################################
//    ラベル呼び出し
//##########################################################
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
		caller.setJumpDest();
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
void Call_declare(void)
{
	CPrmInfo prminfo;
	CreatePrmInfo( &prminfo );
	DeclarePrmInfo( prminfo );
	return;
}

//##########################################################
//    引数情報取得
//##########################################################
//------------------------------------------------
// 呼び出し引数情報を取得する関数
//------------------------------------------------
int Call_arginfo(void** ppResult)
{
	CCall* pCall = TopCallStack();
	int iArg;
	ARGINFOID id;
	
	id   = static_cast<ARGINFOID>( code_geti() );	// データの種類
	iArg = code_getdi(-1);							// 引数の番号
	
	return SetReffuncResult( ppResult, pCall->getArgInfo(id, iArg) );
}

//------------------------------------------------
// 引数の値を取得する
//------------------------------------------------
int Call_argv(void** ppResult)
{
	CCall* pCall = TopCallStack();
	
	PVal* pvalArg;
	vartype_t rettype;
	
	int iArg = code_getdi( 0 );		// 引数の番号
	
	// 返値を設定する
	pvalArg   = pCall->getArgv( iArg );
	rettype   = pvalArg->flag;				// 返値の型を設定
	*ppResult = GetPValRealPtr( pvalArg );	// 実体ポインタを求める
	
	return rettype;
}

//------------------------------------------------
// ラベル関数の返値を設定する
//------------------------------------------------
void Call_retval(void)
{
	CCall* pCall = TopCallStack();
	
	if ( code_getprm() <= PARAM_END ) {
		puterror( HSPERR_NO_DEFAULT );
	}
	
	vartype_t vt      = mpval->flag;
	void* pValue = GetPValRealPtr( mpval );
	pCall->setRetVal( pValue, vt );
	return;
}

//------------------------------------------------
// ラベル関数の返値となった値を取り出す
// 
// @ call 終了後に呼び出される。
//------------------------------------------------
int Call_result(void** ppResult)
{
	PVal* pvResult = CCaller::getLastRetVal();
	
	*ppResult = pvResult->pt;
	return pvResult->flag;
}


//------------------------------------------------
// エイリアスにする
//------------------------------------------------
void Call_alias(void)
{
	CCall* pCall = TopCallStack();
	PVal* pval;
	
	// 引数の取得
	code_getva( &pval );
	int iArg = code_getdi( 0 );
	
	// エイリアス化する
	pCall->alias( pval, iArg );
	return;
}


//------------------------------------------------
// エイリアス名を一括して付ける
//------------------------------------------------
void Call_aliasAll(void)
{
	CCall* pCall = TopCallStack();
	PVal  *pval;
	
	// 列挙された変数をエイリアスにする
	for( int i = 0
		; code_isNextArg() && ( i < pCall->getCntArg() )
		; ++ i
	) {
		// 変数でなければ無視する
		if ( *type != TYPE_VAR ) {
			code_getprm();
			
		// 変数を取得
		} else {
			code_getva( &pval );
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
	return SetReffuncResult( ppResult, TopCallStack()->getLabel() );
}


//##########################################################
//    引数ストリーム呼び出し
//##########################################################
//------------------------------------------------
// 引数ストリーム呼び出し::開始
//------------------------------------------------
void Call_StreamBegin(void)
{
	// 呼び出し前の処理
	CCaller *pCaller = new CCaller;
	
	g_stkStream.push( pCaller );
	
	// ラベルの設定
	if ( code_isNextArg() ) {
		Call_StreamLabel();
	}
	return;
}


//------------------------------------------------
// 引数ストリーム呼び出し::ラベル設定
//------------------------------------------------
void Call_StreamLabel(void)
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );
	
	CCaller *pCaller = g_stkStream.top();
	
	// ジャンプ先の決定
	pCaller->setJumpDest();
	return;
}

//------------------------------------------------
// 引数ストリーム呼び出し::追加
//------------------------------------------------
void Call_StreamAdd(void)
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );
	
	CCaller *pCaller = g_stkStream.top();
	
	// 引数を追加する
	pCaller->setArgAll();
	
	return;
}


//------------------------------------------------
// 引数ストリーム呼び出し::完了
// 
// @ 命令形式の場合は ppResult == NULL 。
//------------------------------------------------
int Call_StreamEnd(void** ppResult)
{
	if ( g_stkStream.empty() ) puterror( HSPERR_NO_DEFAULT );
	
	CCaller *pCaller = g_stkStream.top();
	
	// 呼び出し
	pCaller->call();
	
	// 後処理
	g_stkStream.pop();
	
	vartype_t restype = pCaller->getCallResult( ppResult );
	
	delete pCaller;
	
	return restype;
}

//##########################################################
//    ユーザ定義命令関係
//##########################################################
//------------------------------------------------
// ユーザ定義命令・関数の id を取得して加工する
//------------------------------------------------
int DefIdOf(void** ppResult)
{
	int deffid = code_getdefid();
	
	if ( deffid == 0 ) puterror( HSPERR_TYPE_MISMATCH );
	
	return SetReffuncResult( ppResult, deffid );
}

//------------------------------------------------
// ユーザ定義命令・関数からラベルを取得する
//------------------------------------------------
int LabelOf(void** ppResult)
{
	int deffid ( 0 );
	label_t lb ( NULL );
	
	// ユーザ定義命令・関数
	if ( *type == TYPE_MODCMD ) {
		deffid = code_getdefid();
		
	// その他
	} else {
		if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
		
		// deffid
		if ( mpval->flag == HSPVAR_FLAG_INT ) {
			deffid = *reinterpret_cast<int*>( mpval->pt );
			
		// label
		} else if ( mpval->flag == HSPVAR_FLAG_LABEL ) {
			lb = *reinterpret_cast<label_t *>( mpval->pt );
			
		// caller
		} else if ( mpval->flag == HSPVAR_FLAG_CALLER ) {
			lb = GetLabel( reinterpret_cast<StCaller* >( mpval->pt )->dest );
		}
	}
	
	if ( !lb ) {
		if ( isModcmdId(deffid) ) {
			STRUCTDAT* pStDat = GetSTRUCTDAT( deffid );
			
			lb = GetLabel_fromOTindex( pStDat->otindex );
			
		} else {
			puterror( HSPERR_ILLEGAL_FUNCTION );
		}
	}
	
	return SetReffuncResult( ppResult, lb );
}

//##########################################################
//    caller 型関係
//##########################################################
//------------------------------------------------
// 型変換関数
//------------------------------------------------
int Caller_cnv(void** ppResult)
{
	HspVarProc* vp = GetHspVarProc( HSPVAR_FLAG_CALLER );
	
	void* p;
	vartype_t vt;
	
	// #deffunc コマンドの場合
	if ( *type == TYPE_MODCMD ) {
		g_resIntVal = code_getdefid();
		
		p  = &g_resIntVal;
		vt = HSPVAR_FLAG_INT;
		
	// それ以外
	} else {
		int prm = code_getprm();
		if ( prm <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
		
		p  = mpval->pt;
		vt = mpval->flag;
	}
	
	// 変換して格納
	*ppResult = vp->Cnv( p, vt );
	
	return HSPVAR_FLAG_CALLER;
}

//##########################################################
//    コマンド返値
//##########################################################
//------------------------------------------------
// int 型の値を返値として返す
//------------------------------------------------
int SetReffuncResult(void** ppResult, int n)
{
	return SetReffuncResult_core<int, HSPVAR_FLAG_INT>( ppResult, n );
}

//------------------------------------------------
// label 型の値を返値として返す
//------------------------------------------------
int SetReffuncResult(void** ppResult, label_t lb)
{
	return SetReffuncResult_core<label_t, HSPVAR_FLAG_LABEL>( ppResult, lb );
}

//##########################################################
//    その他
//##########################################################
//------------------------------------------------
// コマンドの呼び出し (命令形式)
//------------------------------------------------
void CallCmd_sttm(void)
{
	int id = code_geti();
	
	if ( !(id & DefId::MagicCode) ) puterror( HSPERR_ILLEGAL_FUNCTION );
	
	{
		*type = DefId::getType( id );
		*val  = DefId::getCode( id );
		*exinfo->npexflg = EXFLG_1;
	}
	
	return;
}

//------------------------------------------------
// コマンドの呼び出し (関数形式)
//------------------------------------------------
int CallCmd_func( void** pResult )
{
	int id = code_geti();
	if ( !(id & DefId::MagicCode) ) puterror( HSPERR_ILLEGAL_FUNCTION );
	
	// 関数を呼び出す (その返値を call_func 自体の返値とする)
	{
		*type = DefId::getType( id );
		*val  = DefId::getCode( id );
		*exinfo->npexflg = 0;
		
		if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	}
	
	*pResult = mpval->pt;
	return mpval->flag;
}

//##########################################################
//    テストコード
//##########################################################
#ifdef _DEBUG

void CallHpi_test(void)
{
	
	return;
}

#endif
