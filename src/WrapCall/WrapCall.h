// WrapCall header

#ifndef IG_WRAP_CALL_H
#define IG_WRAP_CALL_H
#ifdef with_WrapCall

#include "../main.h"

namespace WrapCall
{

struct ModcmdCallInfo;
using stkCallInfo_t = std::vector<std::unique_ptr<ModcmdCallInfo const>>;
using stkCallInfoRange_t = pair_range<stkCallInfo_t::const_iterator>;

extern void bgnCall(stdat_t callee);
extern void endCall();
extern void endCall(PDAT* p, vartype_t vtype);

extern optional_ref<ModcmdCallInfo const> getCallInfoAt(size_t idx);
extern stkCallInfoRange_t getCallInfoRange();

};

#endif
#endif
