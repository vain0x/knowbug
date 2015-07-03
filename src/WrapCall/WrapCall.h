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

extern void bgnCall(stdat_t callee);
extern void endCall();
extern void endCall(PDAT* p, vartype_t vtype);

extern ModcmdCallInfo::shared_ptr_type tryGetCallInfoAt(size_t idx);
extern stkCallInfoRange_t getCallInfoRange();

};

#endif
#endif
