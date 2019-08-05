#pragma once

#include <optional>
#include "hsx_types_fwd.h"

namespace hsp_sdk_ext {
	// デバッグセグメントに埋め込まれた情報の種類
	enum class DebugSegmentItemKind {
		SourceFile,
		VarName,
		LabelName,
		ParamName,
	};

	// デバッグセグメントに埋め込まれた情報
	class DebugSegmentItem {
		DebugSegmentItemKind kind_;

		char const* str_;

		int num_;

	public:
		DebugSegmentItem(DebugSegmentItemKind kind, char const* str, int num)
			: kind_(kind)
			, str_(str)
			, num_(num)
		{
		}

		auto kind() const -> DebugSegmentItemKind {
			return kind_;
		}

		// ファイル参照名、ラベル名、パラメータ名のいずれか
		auto str() const -> char const* {
			return str_;
		}

		// 行番号 (1-indexed), object temp index, param index のいずれか
		auto num() const -> int {
			return num_;
		}
	};

	// デバッグセグメントに記録された識別子の種類。
	// 種類は出現する文脈によって決まる。
	enum class DebugSegmentIdentKind {
		VarName,
		LabelName,
		ParamName,
		None,
	};

	// デバッグセグメントから情報を読み取るもの。
	// 参照: hspsdk/hsp3code.txt, `CToken::PutDI` (hspcmp/codegen.cpp)
	class DebugSegmentReader {
		HSPCTX const* ctx_;

		// デバッグセグメント上の位置。
		std::size_t di_;

		// 識別子文脈。
		// 次に出現した識別子の種類がどれかを表す。
		DebugSegmentIdentKind ident_kind_;

		// コードセグメント上の位置。
		// ソースファイル位置とコード位置の対応を取るために使う。
		HspCodeUnit const* cs_;

	public:
		explicit DebugSegmentReader(HSPCTX const* ctx);

		// デバッグセグメントを読み進める。
		// 次に何らかの情報を発見したら停止して、その情報を返す。
		// 終端に達したら nullopt を返す。
		auto next()->std::optional<DebugSegmentItem>;
	};
}
