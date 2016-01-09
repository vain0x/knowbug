// WrapCall header

#ifndef IG_WRAP_CALL_H
#define IG_WRAP_CALL_H
#ifdef with_WrapCall

#include "../main.h"
#include "ModcmdCallInfo.h"

namespace WrapCall
{

using stkCallInfo_t = std::vector<ModcmdCallInfo::shared_ptr_type>;
using stkCallInfoRange_t = pair_range<stkCallInfo_t::const_iterator>;

extern auto tryGetCallInfoAt(size_t idx) -> ModcmdCallInfo::shared_ptr_type;
extern auto getCallInfoRange() -> stkCallInfoRange_t;

};

#endif
#endif
