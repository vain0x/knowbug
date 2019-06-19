#pragma once

#include "hpiutil/hpiutil_fwd.hpp"
#include "HspDebugApi.h"
#include "HspStaticVars.h"

// HSP 関連の操作をまとめるもの。
// HSP 側から取得できる情報を knowbug 用に加工したりキャッシュしたりする機能を持つ (予定)
class HspRuntime {
	HspDebugApi api_;
	HspStaticVars static_vars_;

public:
	HspRuntime(HspDebugApi&& api_);

	auto static_vars() -> HspStaticVars& {
		return static_vars_;
	}

private:
	auto ctx() -> HSPCTX*;

	auto debug() -> HSP3DEBUG*;

	auto exinfo() -> HSPEXINFO*;
};
