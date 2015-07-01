// WrapCall

#include <vector>

#include "WrapCall.h"
#include "type_modcmd.h"
#include "DbgWndMsg.h"
#include "WrapCallSdk.h"

namespace WrapCall
{

typedef std::vector<ModcmdCallInfo*> infCalling_t;

struct WrapCallData
{
	struct lastResult_t
	{
		void*      p;
		vartype_t vt;
	};
	
	infCalling_t* pInfCalling;
	lastResult_t  lastResult;
	
	WrapCallMethod methods;
	
public:
	WrapCallData()
	{
		pInfCalling   = new infCalling_t;
		lastResult.p  = NULL;
		lastResult.vt = HSPVAR_FLAG_NONE;
		return;
	}
	
	~WrapCallData()
	{
		delete pInfCalling; pInfCalling = NULL;
		return;
	}
	
	bool        empty(void) const { return pInfCalling->empty(); }
	unsigned int size(void) const { return pInfCalling->size();  }
};

// 変数定義
extern WrapCallData* g_pWrapCallData = NULL;
static HWND          g_hDbgWnd       = NULL;
static HSP3DEBUG*    g_pDebug        = NULL;

// 関数宣言
static void setLastResult( void* p, vartype_t vt );

static void PutLogMsg( const char* log );
static void PutLogMsgWarning( const char* log );

//------------------------------------------------
// WrapCall の準備
// 
// @ 実行開始時に呼ばれる (WrapCall_init 命令)。
//------------------------------------------------
void init(void)
{
	g_hDbgWnd = reinterpret_cast<HWND>( exinfo->er );
	g_pDebug  = reinterpret_cast<HSP3DEBUG*>(
		SendMessage( g_hDbgWnd, DWM_RequireDebugStruct, 0, 0 )
	);
	
	g_pWrapCallData = new WrapCallData;
	
	SendMessage( g_hDbgWnd, DWM_RequireMethodFunc, 0, (LPARAM)(&g_pWrapCallData->methods) );
	return;
}

//------------------------------------------------
// WrapCall の破棄
// 
// @ 実行終了後に呼ばれる。
//------------------------------------------------
void term(void)
{
	delete g_pWrapCallData; g_pWrapCallData = NULL;
	return;
}

//##############################################################################
//                アクション
//##############################################################################

static char stt_buf[1024];	// 一時バッファ

//------------------------------------------------
// 呼び出しの開始
//------------------------------------------------
void bgnCall( STRUCTDAT* pStDat )
{
	ModcmdCallInfo* pCallInfo = new ModcmdCallInfo;
	
	pCallInfo->sublev     = ctx->sublev;
	pCallInfo->looplev    = ctx->looplev;
	pCallInfo->pStDat     = pStDat;
	pCallInfo->prmstk_bak = ctx->prmstack;
	pCallInfo->fname      = g_pDebug->fname;
	pCallInfo->line       = g_pDebug->line;
	
	g_pWrapCallData->pInfCalling->push_back( pCallInfo );
	unsigned int idx = g_pWrapCallData->size() - 1;
	
	// ログ出力
	/*
	{
		printf( stt_buf, "[CallBgn] %s\t@%d of \"%s\"]",
			&ctx->mem_mds[pCallInfo->pStDat->nameidx],
			pCallInfo->line,
			pCallInfo->fname
		);
		PutLogMsg( stt_buf );
	}
	//*/
	
	// DebugWindow への通知
	g_pWrapCallData->methods.BgnCalling( idx, pCallInfo );
	
	return;
}

//------------------------------------------------
// 呼び出しの完了
//------------------------------------------------
void endCall( void* p, vartype_t vt )
{
	if ( g_pWrapCallData->empty() ) return;
	
	unsigned int idx = g_pWrapCallData->size() - 1;
	const ModcmdCallInfo* pCallInfo = g_pWrapCallData->pInfCalling->at( idx );
	
	// エラーの指摘
	if ( ctx->looplev != pCallInfo->looplev ) {
		PutLogMsgWarning( "呼び出し中に入った loop から、正常に脱出せず、呼び出しが終了した。" );
	}
	
	if ( ctx->sublev != pCallInfo->sublev ) {
		PutLogMsgWarning( "呼び出し中に入ったサブルーチンから、正常に脱出せず、呼び出しが終了した。" );
	}
	
	// ログ出力
	/*
	{
		sprintf_s( stt_buf, "[CallEnd] %s", &ctx->mem_mds[pCallInfo->pStDat->nameidx] );
		PutLogMsg( stt_buf );
	}
	//*/
	
	// (あれば) 返値の更新
	if ( p != NULL ) setLastResult( p, vt );
	
	// DebugWindow への通知
	g_pWrapCallData->methods.EndCalling( idx, pCallInfo );
	
	// 呼び出しデータの除去
	g_pWrapCallData->pInfCalling->pop_back();
	delete pCallInfo;
	
	return;
}

//------------------------------------------------
// 最後の返値を設定する
//------------------------------------------------
void setLastResult( void* p, vartype_t vt )
{
	g_pWrapCallData->lastResult.p  = p;
	g_pWrapCallData->lastResult.vt = vt;
	return;
}

//##############################################################################
//                DebugWindow との通信
//##############################################################################
//------------------------------------------------
// ログメッセージの追加
// 
// @prm wp: --
// @prm lp: メッセージへのポインタ
//------------------------------------------------
void PutLogMsg( const char* log )
{
	strncpy( ctx->stmp, log, 0x400 - 1 );
	g_pWrapCallData->methods.AddLog( ctx->stmp );
	return;
}

void PutLogMsgWarning( const char* log )
{
	sprintf( ctx->stmp, "[Warning] %*s", log );
	g_pWrapCallData->methods.AddLog( ctx->stmp );
	return;
}

}
