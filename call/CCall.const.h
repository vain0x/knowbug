// CCall - const

#ifndef IG_CLASS_CALL_CONST_H
#define IG_CLASS_CALL_CONST_H

// CCall が使用する定数で、公開されるもの。

//------------------------------------------------
// arginfo dataID
//------------------------------------------------
enum ARGINFOID
{
	ARGINFOID_FLAG = 0,		// vartype
	ARGINFOID_MODE,			// 変数モード( 0 = 未初期化, 1 = 通常, 2 = クローン )
	ARGINFOID_LEN1,			// 一次元目要素数
	ARGINFOID_LEN2,			// 二次元目要素数
	ARGINFOID_LEN3,			// 三次元目要素数
	ARGINFOID_LEN4,			// 四次元目要素数
	ARGINFOID_SIZE,			// 全体のバイト数
	ARGINFOID_PTR,			// 実体へのポインタ
	ARGINFOID_BYREF,		// 参照渡しか
	ARGINFOID_NOBIND,		// nobind 引数か
	ARGINFOID_MAX
};

#endif
