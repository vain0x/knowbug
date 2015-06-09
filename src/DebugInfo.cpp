
#include "main.h"
#include "hspwnd.h"
#include "DebugInfo.h"
#include "CAx.h"
#include "module/supio/supio.h"

using namespace hpimod;

DebugInfo::DebugInfo(HSP3DEBUG* debug)
	: debug(debug)
	, ax(new CAx())
{ }

DebugInfo::~DebugInfo() {}

string DebugInfo::getCurInfString() const
{
	return formatCurInfString(debug->fname, debug->line);
}

string DebugInfo::formatCurInfString(char const* fname, int line)
{
	return strf("#%d \"%s\"", line, (fname ? fname : "(nameless)"));
}

std::vector<std::pair<string, string>> DebugInfo::fetchGeneralInfo() const
{
	std::vector<std::pair<string, string>> res;
	res.reserve(20);

	// HSP側に問い合わせ
	std::unique_ptr<char, void(*)(char*)> p(
		debug->get_value(DEBUGINFO_GENERAL),
		debug->dbg_close
	);

	strsp_ini();
	for ( ;; ) {
		char name[0x100];
		char val[0x200];
		{
			int const chk = strsp_get(p.get(), name, 0, sizeof(name) - 1);
			if ( chk == 0 ) break;
		}
		{
			int const chk = strsp_get(p.get(), val, 0, sizeof(val) - 1);
			if ( chk == 0 ) break;
		}
		res.emplace_back(name, val);
	}

	// 拡張内容の追加
	if ( exinfo->actscr ) {
		auto const pBmscr = reinterpret_cast<BMSCR*>(exinfo->HspFunc_getbmscr(*exinfo->actscr));
		if ( pBmscr ) {
			// color
			{
				COLORREF const cref = pBmscr->color;
				res.emplace_back("color",
					strf("(%d, %d, %d)", GetRValue(cref), GetGValue(cref), GetBValue(cref)));
			}
			// pos
			res.emplace_back("pos", strf("(%d, %d)", pBmscr->cx, pBmscr->cy));
		}
	}
	return std::move(res);
}

std::vector<string> DebugInfo::fetchStaticVarNames() const
{
	std::vector<string> res;
	res.reserve(hpimod::cntSttVars());

	std::unique_ptr<char, void(*)(char*)> p(
		debug->get_varinf(nullptr, 0xFF),
		debug->dbg_close
	);
	strsp_ini();
	for ( ;; ) {
		char name[0x100];
		int const chk = strsp_get(p.get(), name, 0, sizeof(name) - 1);
		if ( chk == 0 ) break;
		res.emplace_back(name);
	}
	return std::move(res);
}
