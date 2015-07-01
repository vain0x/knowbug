// DebugWindowMessage

#ifndef IG_CONST_DEBUG_WINDOW_MESSAGE_H
#define IG_CONST_DEBUG_WINDOW_MESSAGE_H

#include <windows.h>

//------------------------------------------------
// デバッグウィンドウへ送るメッセージのID
//------------------------------------------------
enum DebugWindowMessage
{
	DWM_Bgn   = WM_USER + 0x0080,
	
	DWM_RequireDebugStruct,		// HSP3DEBUG* の要求
	DWM_RequireMethodFunc,		// WrapCallMethod に関数ポインタを設定することを要求 (lp: WrapCallMethod*)
	
	DWM_PutLogMsg,			// ログメッセージへの追加
	DWM_OnBgnCalling,		// コマンド呼び出し開始時 (wp: idx, lp: ModcmdCallInfo*)
	DWM_OnEndCalling,		// コマンド呼び出し終了時 (wp: idx, lp: ModcmdCallInfo*)
	DWM_OnResultReturning,	// コマンド呼び出し返値通知 (wp: vartype_t, lp: void*)
	
	DWM_End,
};

#endif
