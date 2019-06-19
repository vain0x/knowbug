//! HSP プラグイン API の薄いラッパー

#pragma once

#include "hpiutil/hpiutil_fwd.hpp"

// HSP がデバッガー用に公開している情報を扱いやすくするための薄いラッパー。
class HspDebugApi {
	HSPCTX* context_;

	HSP3DEBUG* debug_;

	HSPEXINFO* exinfo_;

public:
	HspDebugApi(HSP3DEBUG* debug);

	auto context() -> HSPCTX* {
		return context_;
	}

	auto debug() -> HSP3DEBUG* {
		return debug_;
	}

	auto exinfo() -> HSPEXINFO* {
		return exinfo_;
	}
};
