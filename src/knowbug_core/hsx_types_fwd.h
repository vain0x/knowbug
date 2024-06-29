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

namespace hsx {
	class HspData;
	class HspDimIndex;
	class HspParamData;
	class HspParamStack;
	class HspVarMetadata;

	using HspDouble = double;

	using HspInt = std::int32_t;

	// 引数の種類
	// MPTYPE_*
	using HspParamType = int;

	// HSP の変数が持つデータの型
	// FIXME: HspVarType に改名
	enum class HspType {
		None = HSPVAR_FLAG_NONE,
		Label = HSPVAR_FLAG_LABEL,
		Str = HSPVAR_FLAG_STR,
		Double = HSPVAR_FLAG_DOUBLE,
		Int = HSPVAR_FLAG_INT,
		Struct = HSPVAR_FLAG_STRUCT,
		Comstruct = HSPVAR_FLAG_COMSTRUCT,
	};

	// HSP の変数のモード
	enum class HspVarMode {
		None = HSPVAR_MODE_NONE,
		Alloc = HSPVAR_MODE_MALLOC,
		Clone = HSPVAR_MODE_CLONE,
	};

	// システム変数の種類。
	// 順番は名前順、ただし類似したものは近くに集める。
	enum class HspSystemVarKind {
		Cnt = 1,
		Err,
		IParam,
		WParam,
		LParam,
		LoopLev,
		SubLev,
		Refstr,
		Refdval,
		Stat,
		StrSize,
		Thismod,
	};

	// デバッグウィンドウへの通知の種類
	enum class DebugNoticeKind {
		// assert, stop やステップ実行の完了などにより、HSP スクリプトの実行が一時停止したとき
		Stop = 0,

		// logmes 命令が実行されたとき。ログの内容は ctx->stmp にある。
		Logmes = 1,
	};
}
