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

namespace hsx {
	class HspData;
	class HspDimIndex;
	class HspParamData;
	class HspParamStack;
	class HspVarMetadata;

	// オブジェクトテンポラリ (ラベルのこと)
	// code segment (CS) 領域のオフセットという形で、コードの位置を表す。
	using HspObjectTemp = std::int32_t;

	// ラベル (ランタイムにおけるラベルの表現)。
	// code segment 内へのポインタ (有効) または nullptr (無効)。
	using HspLabel = HsxCodeUnit const*;

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
