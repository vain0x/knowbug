// 外部関数の呼び出しルーチン

#include "call_extfunc.h"

//------------------------------------------------
// 外部関数呼び出し
// 
// @prm proc : 関数ポインタ
// @prm prm  : 実引数の入った int 配列
// @prm prms : 引数リスト
// @result   : 関数の戻り値
//------------------------------------------------
#ifdef _MSC_VER

// @compiler: VC++
__declspec( naked ) int __cdecl call_extfunc( void *proc, int *prm, int prms )
{
	__asm {
		push	ebp
		mov		ebp,esp

		;# ebp+8	: 関数のポインタ
		;# ebp+12	: 引数が入ったINTの配列
		;# ebp+16	: 引数の数（pushする回数）

		;# パラメータをnp個pushする
		mov		eax, dword ptr [ebp+12]
		mov		ecx, dword ptr [ebp+16]
		jmp		_$push_chk

	_$push:
		push	dword ptr [eax+ecx*4]

	_$push_chk:
		dec		ecx
		jge		_$push

		;# 関数呼び出し
		call	dword ptr [ebp+8]

		;# 戻り値は eax に入るのでそのままリターン
		leave
		ret
	}
}

#elif defined( __GNUC__ )

// @ compiler: gcc
int __cdecl call_extfunc(void * proc, int * prm, int prms)
{
    int ret = 0;
    __asm__ volatile (
		"pushl  %%ebp;"
		"movl   %%esp, %%ebp;"
		"jmp    _push_chk;"
		
		// パラメータをprms個pushする
	"_push:"
		"pushl  ( %2, %3, 4 );"
		
	"_push_chk:"
		"decl   %3;"
		"jge    _push;"
		
		"calll  *%1;"
		"leave;"
		
		: "=a" ( ret )
        : "r" ( proc ) , "r" ( prm ), "r" ( prms )
    );
    return ret;
}

