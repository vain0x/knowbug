#include "pch.h"
#include "hsx_debug_segment.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	// 次の16ビット (2バイト) を整数として読む。
	static auto read_int16(unsigned char const* p) -> unsigned short {
		return (int)*p | ((int)p[1] << 8);
	}

	// 次の24ビット (3バイト) を整数として読む。
	static auto read_int24(unsigned char const* p) -> int {
		return (int)*p | ((int)p[1] << 8) | ((int)p[2] << 16);
	}

	static auto code_segment_data(HSPCTX const* ctx) -> HspCodeUnit const* {
		return ctx->mem_mcs;
	}

	static auto debug_segment_data(HSPCTX const* ctx) -> unsigned char const* {
		return ctx->mem_di;
	}

	static auto debug_segment_size(HSPCTX const* ctx) -> std::size_t {
		return (std::size_t)std::max(0, ctx->hsphed->max_dinfo) / sizeof(unsigned char);
	}

	static auto ident_kind_to_next(DebugSegmentIdentKind kind) -> DebugSegmentIdentKind {
		switch (kind) {
		case DebugSegmentIdentKind::VarName:
			return DebugSegmentIdentKind::LabelName;

		case DebugSegmentIdentKind::LabelName:
			return DebugSegmentIdentKind::ParamName;

		case DebugSegmentIdentKind::ParamName:
		case DebugSegmentIdentKind::None:
			return DebugSegmentIdentKind::None;

		default:
			assert(false && u8"Invalid DebugSegmentIdentKind");
			return DebugSegmentIdentKind::None;
		}
	}

	static auto ident_kind_to_item_kind(DebugSegmentIdentKind kind) -> std::optional<DebugSegmentItemKind> {
		switch (kind) {
		case DebugSegmentIdentKind::VarName:
			return DebugSegmentItemKind::VarName;

		case DebugSegmentIdentKind::LabelName:
			return DebugSegmentItemKind::LabelName;

		case DebugSegmentIdentKind::ParamName:
			return DebugSegmentItemKind::ParamName;

		case DebugSegmentIdentKind::None:
			return std::nullopt;

		default:
			assert(false && u8"Invalid DebugSegmentIdentKind");
			return std::nullopt;
		}
	}

	// -------------------------------------------
	// DebugSegmentReader
	// -------------------------------------------

	DebugSegmentReader::DebugSegmentReader(HSPCTX const* ctx)
		: ctx_(ctx)
		, di_()
		, ident_kind_(DebugSegmentIdentKind::VarName)
		, cs_(code_segment_data(ctx))
	{
	}

	auto DebugSegmentReader::next() -> std::optional<DebugSegmentItem> {
		auto d = debug_segment_data(ctx);

		while (true) {
			if (di_ >= debug_segment_size(ctx)) {
				return std::nullopt;
			}

			switch (d[di_]) {
			case 0xFF: {
				// 文脈の区切り

				ident_kind_ = ident_kind_to_next(ident_kind_);
				di_++;
				continue;
			}
			case 0xFE: {
				// ソースファイル指定: コードセグメントの現在位置に対応するソースファイルの位置が書かれている。

				if (di_ + 6 > debug_segment_size(ctx)) {
					return std::nullopt;
				}

				di_++;

				auto data_index = (int)read_int24(d + di_);
				di_ += 3;

				auto line_number = (int)read_int16(d + di_);
				di_ += 2;

				auto&& file_ref_name_opt = data_as_str(data_index, ctx_);
				if (!file_ref_name_opt) {
					continue;
				}
				return DebugSegmentItem{ DebugSegmentItemKind::SourceFile, *file_ref_name_opt, line_number };
			}
			case 0xFD:
			case 0xFB: {
				// 識別子指定: 現在の文脈で指定された種類の識別子が書かれている。

				if (di_ + 6 > debug_segment_size(ctx)) {
					return std::nullopt;
				}

				di_++;

				auto data_index = (int)read_int24(d + di_);
				di_ += 3;

				auto index = (int)read_int16(d + di_);
				di_ += 2;

				auto&& kind_opt = ident_kind_to_item_kind(ident_kind_);
				auto&& name_opt = data_as_str(data_index, ctx_);
				if (!kind_opt || !name_opt) {
					continue;
				}
				return DebugSegmentItem{ *kind_opt, *name_opt, index };
			}
			case 0xFC: {
				// 次の命令までのCSオフセット値 (3バイト)

				if (di_ + 4 > debug_segment_size(ctx)) {
					return std::nullopt;
				}

				di_++;

				auto offset = read_int24(d + di_);
				di_ += 3;

				cs_ += offset;
				continue;
			}
			default: {
				// 次の命令までの CS オフセット値 (1バイト)

				auto offset = (int)d[di_];
				di_++;

				cs_ += offset;
				continue;
			}
			}
		}
	}
}
