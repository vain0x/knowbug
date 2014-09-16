// call - commad.h

#ifndef IG_CALL_COMMAND_H
#define IG_CALL_COMMAND_H

#include "hsp3plugin_custom.h"

extern int  Call( void** ppResult = nullptr );
extern void Call_retval();
extern void Call_alias();
extern void Call_aliasAll();
extern void Call_declare();

extern void Call_StreamBegin();
extern void Call_StreamLabel();
extern void Call_StreamAdd();
extern int  Call_StreamEnd( void** ppResult = nullptr );

extern int  Call_NewStreamCaller( void** ppResult );
extern void Call_StreamCallerAdd();

extern int  Call_arginfo ( void** ppResult );
extern int  Call_argv    ( void** ppResult );
extern int  Call_getLocal( void** ppResult );
extern int  Call_result  ( void** ppResult );
extern int  AxCmdOf      ( void** ppResult );
extern int  LabelOf      ( void** ppResult );

extern int  Call_thislb( void** ppResult );

extern int  Functor_cnv( void** ppResult );
extern int  Functor_argc( void** ppResult );
extern int  Functor_isFlex( void** ppResult );

extern void CallCmd_sttm();
extern int  CallCmd_func( void** pResult );
extern int  ArgBind( void** ppResult );
extern int   UnBind( void** ppResult );
extern void ReleaseBounds();
extern int  Call_Lambda( void** ppResult );
extern void Call_LambdaBody();						// lambda が内部で用いるコマンド
extern int  Call_LambdaValue( void** ppResult );

extern int  Call_CoCreate( void** ppResult );
extern void Call_CoYieldImpl();

extern void Call_Term();

// テスト
#ifdef _DEBUG

extern void CallHpi_test();

#endif

//#include "strbuf.h"
// strbuf.h 対応マクロ
/*
#define sbAlloc  hspmalloc
#define sbFree   hspfree
#define sbExpand hspexpand
//*/

#endif
