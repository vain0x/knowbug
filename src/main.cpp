
//
//		HSP debug window support functions for HSP3
//				onion software/onitama 2005
//

#include <winapifamily.h>
#include "main.h"
#include "module/strf.h"
#include "module/CStrBuf.h"
#include "DebugInfo.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "config_mng.h"
#include "dialog.h"
#include "vartree.h"
#include "Tree/Node.h"
#include "Tree/TvRepr.h"

static HINSTANCE g_hInstance;
std::unique_ptr<DebugInfo> g_dbginfo;

// ランタイムとの通信
EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 );
EXPORT BOOL WINAPI debug_notice( HSP3DEBUG* p1, int p2, int p3, int p4 );
static void debugbye();

// WrapCall 関連
#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"
#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;
#endif

//------------------------------------------------
// Dllエントリーポイント
//------------------------------------------------
int WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{
	switch ( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			g_hInstance = hInstance;
#if _DEBUG
			if ( GetKeyState(VK_SHIFT) & 0x8000 ) { MessageBox(nullptr, "Attach Me!", "knowbug", MB_OK); }
#endif
			break;
		}
		case DLL_PROCESS_DETACH: debugbye(); break;
	}
	return TRUE;
}

//------------------------------------------------
// debugini ptr  (type1)
//------------------------------------------------
EXPORT BOOL WINAPI debugini( HSP3DEBUG* p1, int p2, int p3, int p4 )
{
	ctx    = p1->hspctx;
	exinfo = ctx->exinfo2;

	g_dbginfo.reset(new DebugInfo(p1));
	g_config.initialize();

	Dialog::createMain();

	static auto tvrepr =
		std::make_unique<DataTree::TvRepr>(Dialog::getVarTreeHandle());
	auto& root = DataTree::NodeRoot::instance();
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
		// 実行が停止した (assert、ステップ実行の完了時など)
		case hpimod::DebugNotice_Stop: {
			if (Knowbug::continueConditionalRun()) break;

			g_dbginfo->updateCurInf();
#ifdef with_WrapCall
			VarTree::UpdateCallNode();
#endif
			Dialog::update();

			DataTree::NodeRoot::instance().updateStateAll();
			break;
		}
		case hpimod::DebugNotice_Logmes:
			strcat_s(ctx->stmp, HSPCTX_REFSTR_MAX, "\r\n");
			Knowbug::logmes(ctx->stmp);
			break;
	}
	return 0;
}

//------------------------------------------------
// debugbye
//------------------------------------------------
void debugbye()
{
	VarTree::term();
	Dialog::destroyMain();
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
// 実行制御
//------------------------------------------------
void runStop()
{
	g_dbginfo->setStepMode( HSPDEBUG_STOP );
}

void run()
{
	g_dbginfo->setStepMode(HSPDEBUG_RUN);
	bStepRunning = false;
}

void runStepIn() {
	// 本当のステップ実行でのみフラグが立つ
	bStepRunning = true;

	g_dbginfo->setStepMode(HSPDEBUG_STEPIN);
}

void runStepOver() { return runStepReturn( ctx->sublev ); }
void runStepOut()  { return runStepReturn( ctx->sublev - 1 ); }

// ctx->sublev == sublev になるまで step を繰り返す
void runStepReturn(int sublev)
{
	if ( sublev < 0 ) return run();	

	sublevOfGoal = sublev;
	bStepRunning = false;
	g_dbginfo->setStepMode(HSPDEBUG_STEPIN);
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
			g_dbginfo->setStepMode(HSPDEBUG_STEPIN); // stepin を繰り返す
			return true;
		} else {
			sublevOfGoal = -1;
		}
	}
	return false;
}

//------------------------------------------------
// ログ操作
//------------------------------------------------
void logmes( char const* msg )
{
	Dialog::LogBox::add(msg);
}

void logmesWarning(char const* msg)
{
	g_dbginfo->updateCurInf();
	logmes(strf("warning: %s\r\nCurInf:%s\r\n",
		msg, g_dbginfo->getCurInfString()).c_str());
}

#ifdef with_WrapCall
//------------------------------------------------
// WrapCall メソッド
//------------------------------------------------
void onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	VarTree::AddCallNode(callinfo);

	// ログ出力
	if ( Dialog::logsCalling() ) {
		string const logText = strf(
			"[CallBgn] %s\t%s]\r\n",
			hpimod::STRUCTDAT_getName(callinfo->stdat),
			DebugInfo::formatCurInfString(callinfo->fname, callinfo->line)
		);
		Knowbug::logmes(logText.c_str());
	}
}

void onEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vtype)
{
	VarTree::RemoveLastCallNode();

	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、今作るしかない
	auto const pResult =
		(usesResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
		? std::make_shared<ResultNodeData>(callinfo, ptr, vtype)
		: nullptr;

	if ( pResult ) {
		VarTree::AddResultNode(callinfo, pResult);
	}

	// ログ出力
	if ( Dialog::logsCalling() ) {
		string const logText = strf(
			"[CallEnd] %s%s\r\n",
			hpimod::STRUCTDAT_getName(callinfo->stdat),
			(pResult ? ("-> " + pResult->lineformedString) : "")
		);
		Knowbug::logmes(logText.c_str());
	}
}
#endif

} //namespace Knowbug

//##############################################################################
//                スクリプト向けのAPI
//##############################################################################
EXPORT void WINAPI knowbug_writeVarinfoString(char const* name, PVal* pvalSrc, PVal* pvalDest)
{
	auto const varinf = std::make_unique<CVarinfoText>();
	varinf->addVar(pvalSrc, name);

	if ( pvalDest->flag != HSPVAR_FLAG_STR ) {
		if ( pvalDest->offset != 0 ) puterror(HSPERR_TYPE_MISMATCH);
		exinfo->HspFunc_dim(pvalDest, HSPVAR_FLAG_STR, 0, 1, 0, 0, 0);
	}
	code_setva(pvalDest, pvalDest->offset, HSPVAR_FLAG_STR, varinf->getString().c_str());
}

//------------------------------------------------
// 最後に呼び出された関数の名前 (refstr に出力)
//
// @prm n : 最後の n 個は無視する
//------------------------------------------------
EXPORT void WINAPI knowbug_getCurrentModcmdName(char const* strNone, int n, char* prefstr)
{
#ifdef with_WrapCall
	auto const&& range = WrapCall::getCallInfoRange();
	if ( std::distance(range.first, range.second) > n ) {
		auto const stdat = (*(range.second - (n + 1)))->stdat;
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, hpimod::STRUCTDAT_getName(stdat));
	} else {
		strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
	}
#else
	strcpy_s(prefstr, HSPCTX_REFSTR_MAX, strNone);
#endif
}
