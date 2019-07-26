#pragma once

#include "hsx_data.h"
#include "hsx_dim_index.h"
#include "hsx_param_data.h"
#include "hsx_param_stack.h"
#include "hsx_types_fwd.h"
#include "hsx_var_metadata.h"
#include "memory_view.h"

namespace hsp_sdk_ext {
	extern auto data_from_label(HspLabel* ptr)->HspData;

	extern auto data_from_str(HspStr value)->HspData;

	extern auto data_from_double(HspDouble* ptr)->HspData;

	extern auto data_from_int(HspInt* ptr)->HspData;

	extern auto element_to_indexes(PVal const* pval, std::size_t aptr)->std::optional<HspDimIndex>;

	extern auto element_to_aptr(PVal const* pval, HspDimIndex const& indexes)->std::optional<std::size_t>;

	extern auto element_to_data(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->std::optional<HspData>;

	extern auto element_to_memory_block(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->MemoryView;

	extern auto pval_to_type(PVal const* pval)->HspType;

	extern auto pval_to_varmode(PVal const* pval)->HspVarMode;

	extern auto pval_to_lengths(PVal const* pval)->HspDimIndex;

	extern auto pval_to_element_count(PVal const* pval)->std::size_t;

	extern auto pval_is_standard_array(PVal const* pval, HSPCTX const* ctx)->bool;

	extern auto pval_to_data(PVal const* pval, HSPCTX const* ctx)->std::optional<HspData>;

	extern auto pval_to_memory_block(PVal const* pval, HSPCTX const* ctx)->MemoryView;
}
