#pragma once

#include <array>
#include <utility>

namespace hpiutil {
namespace Sysvar {

struct Info
{
	char const* name;
	short type;
};

//respectively corresponds to List
enum Id
{
	Stat = 0
	, Refstr
	, Refdval
	, Thismod
	, Cnt
	, IParam
	, WParam
	, LParam
	, StrSize
	, Looplev
	, Sublev
	, Err
	, NoteBuf
	, MAX
};

static auto const Count = static_cast<size_t>(Id::MAX);

//respectively corresponds to Id
static auto const List = std::array<Info, Count> {{
	{ "stat"   , HSPVAR_FLAG_INT }
	, { "refstr" , HSPVAR_FLAG_STR }
	, { "refdval", HSPVAR_FLAG_DOUBLE }
	, { "thismod", HSPVAR_FLAG_STRUCT }
	, { "cnt"    , HSPVAR_FLAG_INT }
	, { "iparam" , HSPVAR_FLAG_INT }
	, { "wparam" , HSPVAR_FLAG_INT }
	, { "lparam" , HSPVAR_FLAG_INT }
	, { "strsize", HSPVAR_FLAG_INT }
	, { "looplev", HSPVAR_FLAG_INT }
	, { "sublev" , HSPVAR_FLAG_INT }
	, { "err"    , HSPVAR_FLAG_INT }
	, { "notebuf", HSPVAR_FLAG_STR }
}};

extern Id trySeek(char const* name);
extern int& getIntRef(Id id);
extern FlexValue* tryGetThismod();
extern std::pair<void const*, size_t> tryDump(Id id);

} //namespace Sysvar
} // namespace hpiutil
