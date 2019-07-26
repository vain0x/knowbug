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

namespace hsp_sdk_ext {
	class HspData;
	class HspDimIndex;
	class HspParamData;
	class HspParamStack;
	class HspVarMetadata;

	// code segment の先頭からのオフセット。
	using HspCodeOffset = std::int32_t;

	// code segment 内へのポインタ (有効) または nullptr (無効)
	// このラベルの位置にラベルが書かれているとは限らない (newlab 命令で生成された可能性がある)。
	using HspLabel = unsigned short const*;

	using HspStr = char*;

	using HspDouble = double;

	using HspInt = std::int32_t;

	// 引数の種類
	// MPTYPE_*
	using HspParamType = short;

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
}
