// with modptr.as

#ifndef IG_WITH_MODPTR_AS_H
#define IG_WITH_MODPTR_AS_H
#ifdef with_ModPtr

#include <cassert>

namespace ModPtr {

char const* const VarName_AllInstance = "__all_instances_@__modptr";
int const MagicCode = 0x6B850000;

extern PVal* getAllInstanceVar();
extern FlexValue* getValue(int mp);

inline bool isValid(int mp)
{
	return (mp & 0xFFFF0000) == MagicCode && ModPtr::getAllInstanceVar();
}

inline int getIdx(int mp)
{
	assert(isValid(mp));
	return mp & 0x0000FFFF;
}

};

#endif	// defined(with_ModPtr)
#endif
