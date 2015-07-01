
//
//		HSP debug window support functions for HSP3
//				onion software/onitama 2005
//

#include <stdio.h>
#include <Windows.h>

#include "module/hspdll.h"
#include "module/supio.h"
#include "module/SortNote.h"
#include "resource.h"
#include "main.h"

#include "../../../../../openhsp/trunk/tools/win32/hsed3_footy2/interface.h"

// グローバル変数
HSP3DEBUG *g_debug;
//HSPCTX*       ctx;
//HSPEXINFO* exinfo;
HINSTANCE g_hInstance;

typedef BOOL (CALLBACK *HSP3DBGFUNC)(HSP3DEBUG *,int,int,int);

//----------------------------------------------------------

#include <deque>
#include <vector>
#include <list>
#include <algorithm>
#include <fstream>
#include "module/mod_cstring.h"
#include "module/mod_cast.h"
#include "ClhspDebugInfo.h"
#include "CVarTree.h"
//#include "CVarinfoTree.h"
#include "CVarinfoText.h"
#include "CVarinfoLine.h"
#include "SysvarData.h"

#include "config_mng.h"
#include "dialog.h"
#include "vartree.h"
#include "WrapCall.h"
#include "with_Script.h"

// グローバル変数
static DebugInfo  g_dbginfo_inst;
       DebugInfo* g_dbginfo      = &g_dbginfo_inst;
static CVarTree*  stt_pSttVarTree = NULL;
//static DynTree_t* stt_pDynTree  = NULL;

static bool stt_bStepRunning = false;		// ステップ実行中 (「脱出」等は除く)
static int stt_sublev_for_stepover = -1;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini( HSP3DEBUG *p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG *p1, int p2, int p3, int p4 );
#ifndef clhsp
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 );
#endif
static void CurrentUpdate( void );

// WrapCall 関連
#ifdef with_WrapCall

std::vector<const ModcmdCallInfo*> g_stkCallInfo;

static void termNodeDynamic();
static void UpdateCallNode( HWND hwndTree );

static void OnBgnCalling( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void OnEndCalling( HWND hwndTree, const ModcmdCallInfo& callinfo );
static void OnResultReturning( HWND hwndTree, const ModcmdCallInfo& callinfo, void* ptr, int flag );

// methods
static void WrapCallMethod_AddLog( const char* log );
static void WrapCallMethod_BgnCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo );
static int  WrapCallMethod_EndCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo );
static void WrapCallMethod_ResultReturning( unsigned int idx, const ModcmdCallInfo* pCallInfo, void* ptr, int flag );

#endif

static void InvokeThread();

//----------------------------------------------------------

//------------------------------------------------
// Dllエントリーポイント
//------------------------------------------------
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH:
			g_hInstance = hInstance;
			break;
		
		case DLL_PROCESS_DETACH:
#ifndef clhsp
			debugbye( g_debug, 0, 0, 0 );
#endif
			Dialog::destroyMain();
			break;
	}
	return TRUE;
}

//##############################################################################
//        デバッグウィンドウ::(runtime から呼ばれる関数)
//##############################################################################
//------------------------------------------------
// debugini ptr  (type1)
//------------------------------------------------
EXPORT BOOL WINAPI debugini( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	// グローバル変数の初期化
	g_debug = p1;
	ctx     = p1->hspctx;
	exinfo  = ctx->exinfo2;
	
	g_dbginfo->debug  = p1;
	g_dbginfo->ctx    = ctx;
	g_dbginfo->exinfo = exinfo;
	
	// 設定を読み込む
	g_config.initialize();
	
//	DynTree::g_dbginfo = g_dbginfo;
	
	// ウィンドウの生成
	HWND hDlg = Dialog::createMain();
	
#ifdef with_WrapCall
	g_dbginfo->exinfo->er = almighty_cast<int*>( hDlg );	// !! 不要になったが後方互換性のために一応置いておく、しばらくしたら消す
#endif
#ifdef with_Script
	/*
	// 特定の名前の静的変数があれば、ダイアログのウィンドウハンドルを格納する
	const int iVar = exinfo->HspFunc_seekvar(WindowHandleHolderName);
	if ( iVar >= 0 ) {
		PVal* pval = &ctx->mem_var[iVar];
		if ( pval != nullptr && pval->flag == HSPVAR_FLAG_INT && pval->mode == HSPVAR_MODE_MALLOC ) {
			*ptr_cast<int*>(pval->pt) = almighty_cast<intptr_t>( hDlg );
		}
	}
	//*/
#endif

	// 実行位置決定スレッド
//	InvokeThread();
	
	return 0;
}

//------------------------------------------------
// debug_notice ptr  (type1)
// 
// @prm p2 : 0 = stop event,
// @       : 1 = send message (logmes)
//------------------------------------------------
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	switch ( p2 ) {
		// 実行が停止した (stop, wait, await, assert など)
		case DebugNotice_Stop:
		{
			if ( stt_sublev_for_stepover >= 0 ) {
				if ( ctx->sublev > stt_sublev_for_stepover ) {
					g_debug->dbg_set( HSPDEBUG_STEPIN );		// stepin を繰り返す
					break;
				} else {
					stt_sublev_for_stepover = -1;	// 終了
				//	g_debug->dbg_set( HSPDEBUG_STOP );
				}
			}
			
#ifdef with_WrapCall
			UpdateCallNode( Dialog::getVarTreeHandle() );	// 呼び出しノード更新
#endif
			
			CurrentUpdate();
			Dialog::update();
			break;
		}
		
		// logmes 命令が呼ばれた
		case DebugNotice_Logmes:
			Knowbug::logmes( ctx->stmp );
			break;
	}
	return 0;
}

//------------------------------------------------
// debugbye ptr  (type1)
// 
// @ clhsp からは呼ばれるが、hsp なら自分で呼ぶ。
//------------------------------------------------
EXPORT BOOL WINAPI debugbye( HSP3DEBUG *p1, int p2, int p3, int p4 )
{
	if ( !g_config->logPath.empty() ) {		// 自動ログ保存
		Dialog::logSave( g_config->logPath.c_str() );
	}
	
#ifdef with_WrapCall
	termNodeDynamic();
#endif
	VarTree::term();
#ifdef with_Script
	termConnectWithScript();
#endif
	if ( stt_pSttVarTree ) {
		delete stt_pSttVarTree; stt_pSttVarTree = NULL;
	}
	return 0;
}

//------------------------------------------------
// 実行中の位置を更新する (line, file)
//------------------------------------------------
void CurrentUpdate( void )
{
	char tmp[512];
	char *fn;
	g_debug->dbg_curinf();
	fn = g_debug->fname;
	if ( fn == NULL ) fn = "???";
	
	sprintf_s( tmp, "%s\n( line : %d )", fn, g_debug->line );
	SetWindowText( Dialog::getSttCtrlHandle(), tmp );
	return;
}

namespace Knowbug
{

//------------------------------------------------
// 実行設定
//------------------------------------------------
void runStop()
{
	g_debug->dbg_set( HSPDEBUG_STOP );
}

void run()
{
	g_debug->dbg_set( HSPDEBUG_RUN );
	stt_bStepRunning = false;
}

void runStepIn() {
	stt_bStepRunning = true;				// 本当のステップ実行でのみフラグが立つ
	g_debug->dbg_set( HSPDEBUG_STEPIN );
}

void runStepOver() { return runStepOut( ctx->sublev ); }
void runStepOut()  { return runStepOut( ctx->sublev - 1 ); }

void runStepOut( int sublev )				// ctx->sublev == sublev になるまで step を繰り返す
{
	if ( sublev < 0 ) return run();			// 最外周への脱出 = 無制限
	stt_sublev_for_stepover = sublev;
	stt_bStepRunning = false;
	g_debug->dbg_set( HSPDEBUG_STEPIN );
	return;
}

//------------------------------------------------
// ログ操作
//------------------------------------------------
void logmes( const char* msg )
{
	Dialog::logAdd( msg );
	Dialog::logAddCrlf();
}

}

//------------------------------------------------
// 静的変数リストを取得する
//------------------------------------------------
CVarTree* getSttVarTree()
{
	CVarTree*& vartree = stt_pSttVarTree;
	
	// 変数リストを作る
	if ( vartree == NULL ) {
		char name[0x100];
		char* p = g_debug->get_varinf( NULL, 0xFF );	// HSP側に問い合わせ
	//	SortNote( p );			// (-) ツリービュー側でソートするので不要
		
		vartree = new CVarTree( "", CVarTree::NodeType_Module );
		
		strsp_ini();
		for (;;) {
			int chk = strsp_get( p, name, 0, 255 );
			if ( chk == 0 ) break;
			
			vartree->push_var( name );
		}
		
		g_debug->dbg_close( p );
	}
	
	return vartree;
}

//##############################################################################
//                WrapCall 関連
//##############################################################################
#ifdef with_WrapCall

// 動的ノードの追加除去の遅延処理はこちらで行う
static size_t g_cntWillAddCallNodes = 0;						// 次の更新で追加すべきノード数
static std::vector<ResultNodeData*> g_willAddResultNodes;		// 次の更新で追加すべき返値ノード
static ResultNodeData* g_willAddResultNodeIndepend = nullptr;	// 〃 ( +dynamic 直下 )

//------------------------------------------------
// prmstack を参照する
//------------------------------------------------
void* ModcmdCallInfo_getPrmstk(const ModcmdCallInfo& callinfo)
{
	if ( callinfo.next == nullptr ) {		// 最新の呼び出し
		if ( ctx->sublev - callinfo.sublev >= 2 ) return nullptr;
		return ctx->prmstack;
	}
	if ( callinfo.isRunning() ) {			// 実行中の呼び出し (引数展開が終了している)
		return callinfo.next->prmstk_bak;
	}
	return nullptr;		// 引数展開中 (スタックフレームが未完成なので prmstack は参照できない)
}

//------------------------------------------------
// Dynamic 関連のデータをすべて破棄する
//------------------------------------------------
void termNodeDynamic()
{
	if ( g_config->bResultNode ) {
		delete g_willAddResultNodeIndepend; g_willAddResultNodeIndepend = nullptr;
		for each ( auto it in g_willAddResultNodes ) delete it;
		g_willAddResultNodes.clear();
	}
	return;
}

//------------------------------------------------
// ResultNodeData の生成
// 
// @ OnEndCaling でしか呼ばれない。
//------------------------------------------------
ResultNodeData* NewResultNodeData( const ModcmdCallInfo& callinfo, void* ptr, int flag )
{
	auto pResult = new ResultNodeData;
		pResult->pStDat      = callinfo.pStDat;
		pResult->sublev      = callinfo.sublev;
		pResult->valueString = "";
		pResult->pCallInfoDepended = callinfo.prev;
	
	{
		auto const varinf = new CVarinfoLine( *g_dbginfo, g_config->maxlenVarinfo );
		varinf->addResult( ptr, flag );
		
		pResult->valueString = varinf->getString();
		
		delete varinf;
	}
	return pResult;		// delete 義務
}

ResultNodeData* NewResultNodeData( const ModcmdCallInfo& callinfo, PVal* pvResult )
{
	return NewResultNodeData( callinfo, pvResult->pt, pvResult->flag );
}

//------------------------------------------------
// 呼び出し開始
//------------------------------------------------
void OnBgnCalling( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	g_stkCallInfo.push_back( &callinfo );

	// ノードの追加
	if ( !stt_bStepRunning ) {
		g_cntWillAddCallNodes ++;		// 後で追加する
	} else {
		VarTree::AddCallNode( hwndTree, callinfo );
	}

	// ログ出力
	if ( Dialog::isLogCallings() ) {
		CString logText = strf(
			"[CallBgn] %s\t@%d of \"%s\"]\n",
			STRUCTDAT_getName(callinfo.pStDat),
			callinfo.line,
			callinfo.fname
		);
		Knowbug::logmes( logText.c_str() );
	}
	
	ctx->retval_level = -1;
	
	return;
}

//------------------------------------------------
// 呼び出し終了
//------------------------------------------------
void OnEndCalling( HWND hwndTree, const ModcmdCallInfo& callinfo )
{
	if ( g_stkCallInfo.empty() ) return;
	
	auto const pResult = g_config->bResultNode && ( ctx->retval_level == ctx->sublev + 1 )
		? NewResultNodeData( callinfo, *exinfo->mpval )
		: nullptr;
	
	// ログ出力
	if ( Dialog::isLogCallings() ) {
		CString logText = strf(
			"[CallEnd] %s%s\n",
			STRUCTDAT_getName(callinfo.pStDat),
			(pResult ? (" -> " + pResult->valueString).c_str() : "")
		);
		Knowbug::logmes( logText.c_str() );
	}
	
	// ノードを削除
	if ( g_cntWillAddCallNodes > 0 ) {
		g_cntWillAddCallNodes --;				// やっぱり追加しない
	} else {
		VarTree::RemoveCallNode( hwndTree, callinfo );	// 既に追加していたので除去される
	}
	
	// 返値ノードの追加
	if ( pResult ) {
		if ( !stt_bStepRunning ) {	// 後で追加する
			if ( pResult->pCallInfoDepended ) {
				g_willAddResultNodes.push_back(pResult);
			} else {
				delete g_willAddResultNodeIndepend;
				g_willAddResultNodeIndepend = pResult;
			}
		} else {
			VarTree::AddResultNode( hwndTree, pResult );
		}
	}
	
	g_stkCallInfo.pop_back();
	return;
}

//------------------------------------------------
// 返値返却
// 
// @ 返値があれば、OnEndCalling の直前に呼ばれる。
// @ ptr, flag はすぐに死んでしまうので、
// @	今のうちに文字列化しておく。
//------------------------------------------------
void OnResultReturning( HWND hwndTree, const ModcmdCallInfo& callinfo, void* ptr, int flag )
{
	return;
}

//------------------------------------------------
// 呼び出しノード更新
//------------------------------------------------
void UpdateCallNode( HWND hwndTree )
{
	// 追加予定の返値ノードを実際に追加する
	if ( g_config->bResultNode && g_willAddResultNodeIndepend ) {
		VarTree::AddResultNode( hwndTree, g_willAddResultNodeIndepend );
		g_willAddResultNodeIndepend = nullptr;
	}
	
	// 追加予定の呼び出しノードを実際に追加する
	if ( g_cntWillAddCallNodes > 0 ) {
		size_t const lenStk = g_stkCallInfo.size() ;
		for ( size_t i = lenStk - g_cntWillAddCallNodes; i < lenStk; ++ i ) {
			VarTree::AddCallNode( hwndTree, *g_stkCallInfo[i] );
		}
		g_cntWillAddCallNodes = 0;
	}
	
	// 追加予定の返値ノードを実際に追加する (2)
	if ( g_config->bResultNode && !g_willAddResultNodes.empty() ) {
		for each ( auto pResult in g_willAddResultNodes ) {
			VarTree::AddResultNode( hwndTree, pResult );
		}
		g_willAddResultNodes.clear();
	}
	return;
}

//------------------------------------------------
// WrapCall メソッド
//------------------------------------------------

static void WrapCallMethod_AddLog( const char* log )
{
	Knowbug::logmes( log );
}

static void WrapCallMethod_BgnCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo )
{
	OnBgnCalling( Dialog::getVarTreeHandle(), *pCallInfo );
}

static int WrapCallMethod_EndCalling( unsigned int idx, const ModcmdCallInfo* pCallInfo )
{
	OnEndCalling( Dialog::getVarTreeHandle(), *pCallInfo );
	return RUNMODE_RUN;
}

static void WrapCallMethod_ResultReturning( unsigned int idx, const ModcmdCallInfo* pCallInfo, void* ptr, int flag )
{
//	OnResultReturning( Dialog::hVarTree, *pCallInfo, ptr, flag );
}

void WrapCall_RequireMethodFunc( WrapCallMethod* methods )
{
	methods->AddLog          = WrapCallMethod_AddLog;
	methods->BgnCalling      = WrapCallMethod_BgnCalling;
	methods->EndCalling      = WrapCallMethod_EndCalling;
	methods->ResultReturning = WrapCallMethod_ResultReturning;
	return;
}

#endif

//------------------------------------------------
// スレッド起動
//------------------------------------------------

class ExecPtrThread
{
public:
	ExecPtrThread() {
		
	}
};

std::map<std::string, std::vector<const unsigned short*>> g_cs_map;		// ファイル名, 行番号から cs 位置を特定する

int tripeek(const unsigned char* p) { return (p[0] << 16 | p[1] << 8 | p[0]); }

void AnalyzeDInfo()
{
	const unsigned short* cur_cs = ctx->mem_mcs;
	const char* cur_fname;
	int cur_line;
	
	auto const push_point = [&cur_fname, &cur_line, &cur_cs] {
		auto iter = g_cs_map.find( cur_fname );
		if ( iter == g_cs_map.end() ) {
			iter = g_cs_map.insert(
				std::pair<std::string, std::vector<const unsigned short*>>(
					cur_fname, std::vector<const unsigned short*>()
				)
			).first;
		}
		iter->second.insert( iter->second.begin() + cur_line, cur_cs );
	};
	
	for ( int i = 0;; ) {
		switch ( ctx->mem_di[i] ) {
			case 0xFF: return;
			case 0xFE:
			{
				int idxDs = tripeek( &ctx->mem_di[i + 1] );
				int line  = *reinterpret_cast<const unsigned short*>( &ctx->mem_di[i + 4] );
				i += 6;
				
				if ( idxDs != 0 ) { cur_fname = &ctx->mem_mds[idxDs]; }
				cur_line = line;
				break;
			}
			case 0xFD: i += 6; break;		// 変数名指定 (無視)
			case 0xFC:
				cur_cs += *reinterpret_cast<const unsigned short*>( &ctx->mem_di[i + 1] );
				push_point();
				i += 2;
				break;
			default:
				cur_cs += ctx->mem_di[i];
				push_point();
				i ++;
				break;
		}
	}
	return;
}

void threadFunc()
{
	const unsigned short* mcs;
	const unsigned short* mcs_bak;

	std::map<int, bool> is_var_checked;
	
	for (;;) {
		is_var_checked.clear();

		std::vector<const unsigned short*>& vec = g_cs_map.at( g_debug->fname );
		mcs = vec[g_debug->line];
		
		if ( mcs >= mcs_bak ) {		// 順進  (!! 最初の mcs_bak 不定値)
			for ( const unsigned short* p = mcs_bak; p <= mcs; ++ p ) {
				const int c = *p;
				int code;
				if ( c & 0x8000 ) {
					code = *reinterpret_cast<const int*>(p); p += 2;
				} else {
					code = *p; p ++;
				}
				
				if ( (c & CSTYPE) == TYPE_VAR && !is_var_checked[code] ) {
					// 静的変数 code の変数値制約を確認する
					is_var_checked.insert( std::pair<int, bool>(code, true) );
				}
			}
		}
		
		mcs_bak = mcs;
		Sleep(1);
	}
}

void InvokeThread()
{
	AnalyzeDInfo();

}
