//! hsp_sdk_ext で使用する型を定義する。

#pragma once

#include <cassert>
#include <cstdint>
#include "../hspsdk/hsp3debug.h"
#include "../hspsdk/hsp3struct.h"
#include "../hspsdk/hspvar_core.h"

#ifdef _WINDOWS
#include "../hspsdk/hspwnd.h"
#endif

// code segment (CS) 領域のデータ
typedef unsigned short HsxCodeUnit;

// オブジェクトテンポラリ (ラベルのこと)
// code segment (CS) 領域のオフセットという形で、コードの位置を表す
typedef int HsxObjectTemp;

// HSP のラベル型の値
//
// code segment (CS) 領域内へのポインタ (有効) または nullptr (無効)
typedef const HsxCodeUnit* HsxLabel;

// HSP の str 型の値
//
// - (エンコーディングの注意)
//    str 型はランタイムのエンコーディング (shift_jis/utf-8) の文字列だけでなく、
//    ほかのエンコーディングの文字列や任意のバイナリを格納するのにも使われることがある
// - (NULL終端の注意)
//    HSPやC言語の文字列データは末尾にヌル文字 (特殊な1バイトの文字) があり、
//    そこが文字列の終端を表すという慣習がある。しかし常にNULL終端が保障されているわけではない
//    特に、std::strlen などのNULL終端を前提とする関数に、NULL終端でないデータを渡してはいけない
// - (バッファサイズの注意)
//    変数や refstr に由来する文字列データはそのバッファサイズを容易に取得できる
//    一方、str 引数の文字列データはバッファサイズを取得できないが、NULL終端が保証されている
typedef const char* HsxStrPtr;

// HSP の str 型の値 (サイズつき)
typedef struct HsxStrSpan {
	// 文字列データの先頭へのポインタ
	const char* data;

	// データ領域のサイズ
	//
	// - 変数や配列要素の場合、メモリブロックのサイズ (GetBlockSizeのbufsize)
	// - refstr の場合、メモリ領域のサイズ
	// - str パラメータの場合、NULL終端までのサイズ
	size_t size;
} HsxStrSpan;

// HSP の double 型の値
typedef double HsxDouble;

// HSP の int 型の値
typedef int HsxInt;

// 変数の型の番号
//
// - 定数 HSPVAR_FLAG_* の値が入る
//-  HspVarProc::flag や PVal::flag などに格納されている
// - HSP の vartype 関数の値
typedef short HsxVartype;

// 変数モード
//
// - 定数 HSPVAR_MODE_* の値が入る
// - CLONE: dup などで作られたクローン変数の状態
typedef short HsxVarMode;

// #deffunc や #func などのパラメーターの種類
//
// - 定数 MPTYPE_* の値が入る
typedef int HsxMptype;

// システム変数の番号
//
// - 順番は名前順、ただし類似したものは近くに集める
typedef enum HsxSysvarKind {
	HSX_SYSVAR_CNT = 1,
	HSX_SYSVAR_ERR,
	HSX_SYSVAR_IPARAM,
	HSX_SYSVAR_WPARAM,
	HSX_SYSVAR_LPARAM,
	HSX_SYSVAR_LOOPLEV,
	HSX_SYSVAR_SUBLEV,
	HSX_SYSVAR_REFSTR,
	HSX_SYSVAR_REFDVAL,
	HSX_SYSVAR_STAT,
	HSX_SYSVAR_STRSIZE,
	HSX_SYSVAR_THISMOD,
} HsxSysvarKind;

// デバッグウィンドウへの通知の種類
typedef enum HsxDebugNotice {
	// assert, stop やステップ実行の完了などにより、HSP スクリプトの実行が一時停止したとき
	HSX_DEBUG_NOTICE_STOP = 0,

	// logmes 命令が実行されたとき。ログの内容は ctx->stmp にある。
	HSX_DEBUG_NOTICE_LOGMES = 1,
} HsxDebugNotice;

// HSP の変数が持つデータへのポインタ
typedef struct HsxData {
	HsxVartype vartype;

	// label, double, int, struct: 変数が持つバッファの一部へのポインタ
	// str: 文字列自身へのポインタ
	const PDAT* pdat;
} HsxData;

// HSP のパラメーターデータへのポインタ
typedef struct HsxParamData {
	const STRUCTPRM* param;
	size_t param_index;
	const void* param_ptr;

	// データの読み取りが安全か
	bool safety;
} HsxParamData;

// HSP のパラメータスタックへの参照
//
// パラメータスタックは、コマンドの実引数、またはインスタンスのメンバ変数の実データが格納される領域
typedef struct HsxParamStack {
	const STRUCTDAT* struct_dat;

	const void* stack_ptr;
	size_t stack_size;

	// データの読み取りが安全か
	bool safety;
} HsxParamStack;

enum HsxConstants {
	// HSP の多次元配列の最大の次元数
	HSX_MAX_DIM = 4,
};

// HSP の多次元配列 (最大4次元) への添字
//
// 配列変数への添字の値の集まりを表す
// または配列変数の次元ごとの長さの集まりを表す
//
// 例:
//      `a(i)` → `dim = 1, data = { i, 0, 0, 0 }`
//      `a(i, j, k, h)` → `dim = 4, data = { i, j, k, h }`
//      `dim a, w, h` → `dim = 2, data = { w, h, 0, 0 }`
typedef struct HsxIndexes {
	// 次元数 (1～4)
	size_t dim;

	// 次元ごとの添字の値、あるいは次元ごとの長さ
	size_t data[HSX_MAX_DIM];
} HsxIndexes;

namespace hsx {
	class HspVarMetadata;
}
