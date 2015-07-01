// call - commad.h

#ifndef IG_CALL_COMMAND_H
#define IG_CALL_COMMAND_H

#include <windows.h>
#include <stack>

#include "hsp3plugin_custom.h"

extern int  Call         (void** ppResult = NULL);
extern void Call_retval  (void);
extern void Call_alias   (void);
extern void Call_aliasAll(void);
extern void Call_declare (void);

extern void Call_StreamBegin(void);
extern void Call_StreamLabel(void);
extern void Call_StreamAdd  (void);
extern int  Call_StreamEnd  (void** ppResult = NULL);

extern int  Call_arginfo (void** ppResult);
extern int  Call_argv    (void** ppResult);
extern int  Call_result  (void** ppResult);
extern int  DefIdOf     (void** ppResult);
extern int  LabelOf      (void** ppResult);

extern int  Call_thislb(void** ppResult);

extern int  Caller_cnv(void** ppResult);

extern void CallCmd_sttm(void);
extern int  CallCmd_func(void** pResult);

//------------------------------------------------
// 返値設定関数のコア
//------------------------------------------------
template<class T, int T_vartype>
int SetReffuncResult_core(void** ppResult, const T& value)
{
	static T stt_value;
	
	stt_value = value;
	*ppResult = &stt_value;
	return T_vartype;
}

extern int  SetReffuncResult(void** ppResult, int n);
extern int  SetReffuncResult(void** ppResult, label_t lb);

// テスト
#ifdef _DEBUG

extern void CallHpi_test(void);

#endif

//#include "strbuf.h"
// strbuf.h 対応マクロ
#define sbAlloc  hspmalloc
#define sbFree   hspfree
#define sbExpand hspexpand

#endif
