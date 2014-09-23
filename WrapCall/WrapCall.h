// WrapCall header

#ifndef IG_WRAP_CALL_H
#define IG_WRAP_CALL_H
#ifdef with_WrapCall

#include <vector>
#include <memory>

#include "../main.h"
//#include "ModcmdCallInfo.h"

namespace WrapCall
{

struct ModcmdCallInfo;
using stkCallInfo_t = std::vector<std::unique_ptr<ModcmdCallInfo const>>;

extern void bgnCall(stdat_t callee);
extern void endCall();
extern void endCall(PDAT* p, vartype_t vtype);

extern ModcmdCallInfo const* getCallInfoAt(size_t idx);
extern std::pair<stkCallInfo_t::const_iterator, stkCallInfo_t::const_iterator> getCallInfoRange();

};

#endif
#endif
