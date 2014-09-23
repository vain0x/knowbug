
//
//		HSP debug window support functions for HSP3
//				onion software/onitama 2005
//

#include <Windows.h>
#include <cstdio>
#include <deque>
#include <vector>
#include <list>
#include <algorithm>
#include <fstream>

#include "hsed3_footy2/interface.h"

#include "module/supio/supio.h"

#include "resource.h"
#include "main.h"

#include "module/strf.h"
#include "module/ptr_cast.h"
#include "DebugInfo.h"
#include "CAx.h"
#include "CVarTree.h"
//#include "CVarinfoTree.h"
#include "CVarinfoText.h"
#include "SysvarData.h"

#include "config_mng.h"
#include "dialog.h"
#include "vartree.h"

static HINSTANCE g_hInstance;
DebugInfo* g_dbginfo = nullptr;
//HSP3DEBUG* g_debug;
//HSPCTX*       ctx;
//HSPEXINFO* exinfo;

static CVarTree* stt_pSttVarTree = nullptr;
//static DynTree_t* stt_pDynTree = nullptr;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG* p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debugbye( HSP3DEBUG* p1, int p2, int p3, int p4 );

static void InvokeThread();

// WrapCall 関連
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"

using WrapCall::ModcmdCallInfo;

static void OnBgnCalling( HWND hwndTree, ModcmdCallInfo const& callinfo );
static void OnEndCalling( HWND hwndTree, ModcmdCallInfo const& callinfo, void* ptr, int flag );
#endif

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
			debugbye( g_dbginfo->debug, 0, 0, 0 );
			Dialog::destroyMain();

			delete g_dbginfo; g_dbginfo = nullptr;
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
EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	// グローバル変数の初期化
//	g_debug = p1;
	ctx     = p1->hspctx;
	exinfo  = ctx->exinfo2;

	g_dbginfo = new DebugInfo(p1);
	
	// 設定を読み込む
	g_config.initialize();
	
//	DynTree::g_dbginfo = g_dbginfo;
	
	// ウィンドウの生成
	HWND const hDlg = Dialog::createMain();
	
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
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	switch ( p2 ) {
		// 実行が停止した (stop, wait, await, assert など)
		case hpimod::DebugNotice_Stop:
		{
			if (Knowbug::continueConditionalRun()) break;

			g_dbginfo->debug->dbg_curinf();
#ifdef with_WrapCall
			VarTree::UpdateCallNode();
#endif
			Dialog::update();
			break;
		}
		
		// logmes 命令が呼ばれた
		case hpimod::DebugNotice_Logmes:
			Knowbug::logmes( ctx->stmp );
			break;
	}
	return 0;
}

//------------------------------------------------
// debugbye ptr  (type1)
//------------------------------------------------
EXPORT BOOL WINAPI debugbye( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	if ( !g_config->logPath.empty() ) {		// 自動ログ保存
		Dialog::logSave( g_config->logPath.c_str() );
	}
	
	VarTree::term();
#ifdef with_Script
	termConnectWithScript();
#endif
	delete stt_pSttVarTree; stt_pSttVarTree = nullptr;
	return 0;
}

namespace Knowbug
{

// ステップ実行中かどうかのフラグ
// 「脱出」等の条件付き実行は除く。
static bool bStepRunning = false;
bool isStepRunning() { return bStepRunning; }

// 条件付き実行の終了条件となる sublev
static int sublevOfGoal = -1;

//------------------------------------------------
// インスタンスハンドル
//------------------------------------------------
HINSTANCE getInstance()
{
	return g_hInstance;
}

//------------------------------------------------
// 実行設定
//------------------------------------------------
void runStop()
{
	g_dbginfo->debug->dbg_set( HSPDEBUG_STOP );
}

void run()
{
	g_dbginfo->debug->dbg_set( HSPDEBUG_RUN );
	bStepRunning = false;
}

void runStepIn() {
	// 本当のステップ実行でのみフラグが立つ
	bStepRunning = true;

	g_dbginfo->debug->dbg_set( HSPDEBUG_STEPIN );
}

void runStepOver() { return runStepReturn( ctx->sublev ); }
void runStepOut()  { return runStepReturn( ctx->sublev - 1 ); }

// ctx->sublev == sublev になるまで step を繰り返す
void runStepReturn(int sublev)
{
	// 最外周への脱出 = 無制限
	if ( sublev < 0 ) {
		return run();	
	}
	sublevOfGoal = sublev;
	bStepRunning = false;
	g_dbginfo->debug->dbg_set( HSPDEBUG_STEPIN );
	return;
}

//------------------------------------------------
// 条件付き実行を続けるかどうか
//
// (そもそもしていない場合も「やめる」(false)を返す)
//------------------------------------------------
bool continueConditionalRun()
{
	if (sublevOfGoal >= 0) {
		if (ctx->sublev > sublevOfGoal) {
			g_dbginfo->debug->dbg_set(HSPDEBUG_STEPIN);		// stepin を繰り返す
			return true;
		} else {
			sublevOfGoal = -1;	// 終了
		//	g_dbginfo->debug->dbg_set( HSPDEBUG_STOP );
		}
	}
	return false;
}

//------------------------------------------------
// ログ操作
//------------------------------------------------
void logmes( char const* msg )
{
	Dialog::logAdd( msg );
	Dialog::logAddCrlf();
}

void logmesWarning(char const* msg)
{
	logmes(strf("warning: %s", msg).c_str());
	Dialog::logAddCurInf();
}

#ifdef with_WrapCall
//------------------------------------------------
// WrapCall メソッド
//------------------------------------------------
void bgnCalling(ModcmdCallInfo const& callinfo)
{
	// ノードの追加
	VarTree::AddCallNode(callinfo);

	// ログ出力
	if ( Dialog::isLogCallings() ) {
		string const logText = strf(
			"[CallBgn] %s\t#%d of \"%s\"]\n",
			hpimod::STRUCTDAT_getName(callinfo.stdat),
			callinfo.line,
			callinfo.fname
		);
		Knowbug::logmes(logText.c_str());
	}
	return;
}

void endCalling(ModcmdCallInfo const& callinfo, void* ptr, vartype_t vtype)
{
	// 最後の呼び出しノードを削除
	VarTree::RemoveLastCallNode();

	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、今作るしかない
	auto const pResult =
		(utilizeResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
		? std::make_shared<ResultNodeData>(callinfo, ptr, vtype)
		: nullptr;

	// 返値ノードの追加
	if ( pResult ) {
		VarTree::AddResultNode(callinfo, pResult);
	}

	// ログ出力
	if ( Dialog::isLogCallings() ) {
		string const logText = strf(
			"[CallEnd] %s%s\n",
			hpimod::STRUCTDAT_getName(callinfo.stdat),
			(pResult ? ("-> " + pResult->valueString).c_str() : "")
		);
		Knowbug::logmes(logText.c_str());
	}
	return;
}
#endif

}

//------------------------------------------------
// 静的変数リストを取得する
//------------------------------------------------
CVarTree* getSttVarTree()
{
	if ( !stt_pSttVarTree ) {
		auto const tree = new CStaticVarTree::ModuleNode(CStaticVarTree::ModuleName_Global);

		char name[0x100];
		char* const p = g_dbginfo->debug->get_varinf( nullptr, 0xFF );	// HSP側に問い合わせ
	//	SortNote( p );		// ツリービュー側でソートするので不要
		strsp_ini();
		for (;;) {
			int const chk = strsp_get( p, name, 0, 255 );
			if ( chk == 0 ) break;
			
			tree->pushVar( name );
		}
		g_dbginfo->debug->dbg_close( p );

		stt_pSttVarTree = tree;
	}
	
	return stt_pSttVarTree;
}

//##############################################################################
//                スクリプト向けのAPI
//##############################################################################
//------------------------------------------------
// 変数情報文字列 (refstr に出力)
//------------------------------------------------
EXPORT void WINAPI knowbug_getVarinfoString(char const* name, PVal* pval,  char* prefstr)
{
	auto const varinf = std::make_unique<CVarinfoText>();
	varinf->addVar(pval, name);
	strcpy_s(prefstr, HSPCTX_REFSTR_MAX, varinf->getString().c_str());
	return;
}

//------------------------------------------------
// 最後に呼び出された関数の名前 (refstr に出力)
//
// @prm n : 最後の n 個は無視する
//------------------------------------------------
EXPORT void WINAPI knowbug_getCurrentModcmdName(char const* strNone, int n, char* prefstr)
{
#ifdef with_WrapCall
	auto const range = WrapCall::getCallInfoRange();
	if ( std::distance(range.first, range.second) > n ) {
		auto const stdat = (*(range.second - (n + 1)))->stdat;
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, hpimod::STRUCTDAT_getName(stdat));
	} else {
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
	}
#else
	strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
#endif
	return;
}

#if 0
//------------------------------------------------
// スレッド起動
//
// TODO: 作りかけ
//------------------------------------------------

class ExecPtrThread
{
public:
	ExecPtrThread() {
		
	}
};

void threadFunc()
{
	/*
	const unsigned short* mcs;
	const unsigned short* mcs_bak;

	std::map<int, bool> is_var_checked;
	
	for (;;) {
		is_var_checked.clear();

		std::map<int, csptr_t>& vec = g_dbginfo->ax->csMap.at( g_dbginfo->debug->fname );
		mcs = vec[g_dbginfo->debug->line];
		
		if ( mcs >= mcs_bak ) {		// 順進  (!! 最初の mcs_bak 不定値)
			for ( csptr_t p = mcs_bak; p <= mcs; ++ p ) {
				const int c = *p;
				int code;
				if ( c & 0x8000 ) {
					code = *reinterpret_cast<int const*>(p); p += 2;
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
	//*/
}

void InvokeThread()
{
	//
}
#endif
