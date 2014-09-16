// 外部関数の呼び出しルーチン

// dllfunc などの呼び出しに使われているアレ

#ifndef __CALL_EXTENDED_FUNCTION_H__
#define __CALL_EXTENDED_FUNCTION_H__

extern
#ifdef _MSC_VER
	__declspec(naked)
#endif
	int __cdecl call_extfunc(void *proc, int *prm, int prms);

#endif
