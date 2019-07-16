#include <array>
#include <memory>
#include <vector>
#include <utility>
#include "hpiutil/hspsdk/hspwnd.h"
#include "DebugInfo.h"
#include "module/supio/supio.h"
#include "module/strf.h"

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

	strsp_ini();
	for ( ;; ) {
		auto name = std::array<char, 0x100> {};
		auto val  = std::array<char, 0x200> {};
		{
			auto const chk = strsp_get(p.get(), name.data(), 0, name.size() - 1);
			if ( chk == 0 ) break;
		}
		{
			auto const chk = strsp_get(p.get(), val.data(), 0, val.size() - 1);
			if ( chk == 0 ) break;
		}
		info.emplace_back(name.data(), val.data());
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
