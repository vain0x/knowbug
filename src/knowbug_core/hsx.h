#pragma once

#include "hsx_data.h"
#include "hsx_dim_index.h"
#include "hsx_param_data.h"
#include "hsx_param_stack.h"
#include "hsx_types_fwd.h"
#include "hsx_var_metadata.h"

namespace hsp_sdk_ext {
	extern auto data_from_label(HspLabel* ptr)->HspData;

	extern auto data_from_str(HspStr value)->HspData;

	extern auto data_from_double(HspDouble* ptr)->HspData;

	extern auto data_from_int(HspInt* ptr)->HspData;
}
