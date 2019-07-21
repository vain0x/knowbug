#include "pch.h"
#include <array>
#include <memory>
#include <vector>
#include <utility>
#include "../hspsdk/hspwnd.h"
#include "DebugInfo.h"
#include "module/strf.h"
#include "string_split.h"

DebugInfo::DebugInfo(HSP3DEBUG* debug)
	: debug_(debug)
{}

DebugInfo::~DebugInfo()
{}

auto DebugInfo::fetchGeneralInfo() const -> std::vector<std::pair<string, string>>
{
	auto info = std::vector<std::pair<string, string>> {};
	info.reserve(20);

	auto p =
		std::unique_ptr<char, void(*)(char*)>
		{ debug_->get_value(DEBUGINFO_GENERAL)
		, debug_->dbg_close
		};

	auto lines = StringLines{ std::string_view{ p.get() } }.iter();
	while (true) {
		auto&& key_opt = lines.next();
		if (!key_opt) {
			break;
		}

		auto&& value_opt = lines.next();
		if (!value_opt) {
			break;
		}

		info.emplace_back(*key_opt, *value_opt);
	}

	// 拡張内容の追加
	if ( exinfo->actscr ) {
		auto const pBmscr =
			reinterpret_cast<BMSCR*>(exinfo->HspFunc_getbmscr(*exinfo->actscr));
		if ( pBmscr ) {
			// color
			{
				auto const cref = COLORREF { pBmscr->color };
				info.emplace_back("color",
					strf("(%d, %d, %d)", GetRValue(cref), GetGValue(cref), GetBValue(cref)));
			}
			// pos
			info.emplace_back("pos", strf("(%d, %d)", pBmscr->cx, pBmscr->cy));
		}
	}
	return info;
}
