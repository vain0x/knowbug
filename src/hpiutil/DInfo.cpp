
#include "hpiutil.hpp"
#include "DInfo.hpp"
#include "../knowbug_core/hsx_debug_segment.h"

namespace hsx = hsp_sdk_ext;

namespace hpiutil {
	auto DInfo::instance() -> DInfo & {
		static auto const inst = std::unique_ptr<DInfo>{ new DInfo {} };
		return *inst;
	}

	void DInfo::parse() {
		auto reader = hsx::DebugSegmentReader{ ctx };
		while (true) {
			auto&& item_opt = reader.next();
			if (!item_opt) {
				break;
			}
			auto&& item = *item_opt;

			switch (item.kind()) {
			case hsx::DebugSegmentItemKind::SourceFile:
				fileRefNames_.emplace(item.str());
				continue;

			case hsx::DebugSegmentItemKind::VarName:
				continue;

			case hsx::DebugSegmentItemKind::LabelName:
				labelNames_.emplace(item.num(), item.str());
				continue;

			case hsx::DebugSegmentItemKind::ParamName:
				paramNames_.emplace(item.num(), item.str());
				continue;

			default:
				assert(false && u8"Unknown DebugSegmentItemKind");
				return;
			}
		}
	}
} //namespace hpiutil
