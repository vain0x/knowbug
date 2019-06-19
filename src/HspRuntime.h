#pragma once

#include "hpiutil/hpiutil_fwd.hpp"
#include "HspStaticVars.h"

class HspRuntime {
	HSPCTX* ctx_;
	HSP3DEBUG* debug_;

	HspStaticVars static_vars_;

public:
	HspRuntime(HSPCTX* ctx, HSP3DEBUG* debug);

	auto static_vars() -> HspStaticVars& {
		return static_vars_;
	}

private:
	auto ctx() -> HSPCTX* {
		return ctx_;
	}

	auto debug() -> HSP3DEBUG* {
		return debug_;
	}

	auto exinfo() -> HSPEXINFO* {
		return ctx()->exinfo2;
	}
};
