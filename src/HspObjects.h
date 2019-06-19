#pragma once

#include <memory>
#include <string>
#include "HspDebugApi.h"

class HspStaticVars;

// HSP のメモリ空間上に存在するオブジェクトに関する情報を取得・変更するためのインターフェイス
class HspObjects {
	HspDebugApi& api_;
	HspStaticVars& static_vars_;

public:
	HspObjects(HspDebugApi& api, HspStaticVars& static_vars)
		: api_(api)
		, static_vars_(static_vars)
	{
	}

	auto static_var_name(std::size_t static_var_id)->std::string;

	bool static_var_is_array(std::size_t static_var_id);

	auto static_var_to_pval(std::size_t static_var_id)->PVal*;
};
