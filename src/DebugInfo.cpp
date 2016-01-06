
#include "main.h"
#include "hspwnd.h"
#include "DebugInfo.h"
#include "module/supio/supio.h"
#include "module/strf.h"

DebugInfo::DebugInfo(HSP3DEBUG* debug)
	: debug_(debug)
{ }

DebugInfo::~DebugInfo() {}

string DebugInfo::formatCurInfString(char const* fname, int line)
{
	return strf("#%d \"%s\"", (line + 1), (fname ? fname : "(nameless)"));
}

std::vector<std::pair<string, string>> DebugInfo::fetchGeneralInfo() const
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
		char name[0x100];
		char val[0x200];
		{
			auto const chk = strsp_get(p.get(), name, 0, sizeof(name) - 1);
			if ( chk == 0 ) break;
		}
		{
			auto const chk = strsp_get(p.get(), val, 0, sizeof(val) - 1);
			if ( chk == 0 ) break;
		}
		info.emplace_back(name, val);
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
	return std::move(info);
}

std::vector<string> DebugInfo::fetchStaticVarNames() const
{
	auto names = std::vector<string> {};
	names.reserve(hpiutil::staticVars().size());

	auto p =
		std::unique_ptr<char, void(*)(char*)>
		{ debug_->get_varinf(nullptr, 0xFF)
		, debug_->dbg_close
		};
	strsp_ini();
	for ( ;; ) {
		char name[0x100];
		auto const chk = strsp_get(p.get(), name, 0, sizeof(name) - 1);
		if ( chk == 0 ) break;
		names.emplace_back(name);
	}
	return std::move(names);
}
