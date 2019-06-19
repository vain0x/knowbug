#include "hpiutil/hpiutil.hpp"
#include "HspDebugApi.h"

HspDebugApi::HspDebugApi(HSP3DEBUG* debug)
	: debug_(debug)
	, context_(debug->hspctx)
	, exinfo_(debug->hspctx->exinfo2)
{
}
