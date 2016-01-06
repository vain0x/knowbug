// with modptr.as

#ifndef IG_WITH_MODPTR_AS_H
#define IG_WITH_MODPTR_AS_H
#ifdef with_ModPtr

#include <cassert>

namespace ModPtr {

static auto const VarName_AllInstance = "__all_instances_@__modptr";
static auto const MagicCode = 0x6B850000;

extern auto getAllInstanceVar() -> PVal*;
extern auto getValue(int mp) -> FlexValue*;

inline bool isValid(int mp)
{
	return (mp & 0xFFFF0000) == MagicCode && ModPtr::getAllInstanceVar();
}

inline auto getIdx(int mp) -> int
{
	assert(isValid(mp));
	return mp & 0x0000FFFF;
}

};

#endif	// defined(with_ModPtr)
#endif
