// WrapCall header

#ifndef IG_WRAP_CALL_H
#define IG_WRAP_CALL_H
#ifdef with_WrapCall

#include <utility>
#include "../main.h"
#include "ModcmdCallInfo.h"

namespace WrapCall
{

using stkCallInfo_t = std::vector<ModcmdCallInfo::shared_ptr_type>;
using stkCallInfoRange_t = std::pair<stkCallInfo_t::const_iterator, stkCallInfo_t::const_iterator>;

extern auto tryGetCallInfoAt(size_t idx) -> ModcmdCallInfo::shared_ptr_type;
extern auto getCallInfoRange() -> stkCallInfoRange_t;

inline stkCallInfo_t::const_iterator begin(stkCallInfoRange_t const& range)
{
	return range.first;
}
inline stkCallInfo_t::const_iterator end(stkCallInfoRange_t const& range)
{
	return range.second;
}

};

#endif
#endif
