#pragma once

#include "hsx.h"

// 危険な操作に印をつける。
#define UNSAFE(E) (E)

namespace hsp_sdk_ext {
	static auto exinfo(HSPCTX* ctx)->HSPEXINFO* {
		assert(ctx != nullptr);
		return ctx->exinfo2;
	}

	static auto exinfo(HSPCTX const* ctx)->HSPEXINFO const* {
		return exinfo(const_cast<HSPCTX*>(ctx));
	}

	static auto vartype_to_int(HspType vartype)->int {
		return (int)vartype;
	}

	static auto vartype_from_int(int vartype)->HspType {
		return (HspType)vartype;
	}

	static auto vartype_to_varproc(HspType vartype, HSPCTX const* ctx)->HspVarProc* {
		return exinfo(ctx)->HspFunc_getproc(vartype_to_int(vartype));
	}

	// support_flag: HSPVAR_SUPPORT_*
	static auto varproc_does_support(HspVarProc const* varproc, unsigned short support_flag)->bool {
		assert(varproc != nullptr);
		return (varproc->support & support_flag) != 0;
	}

	static auto pval_to_varproc(PVal const* pval, HSPCTX const* ctx)->HspVarProc const* {
		return vartype_to_varproc(pval_to_type(pval), ctx);
	}
}
