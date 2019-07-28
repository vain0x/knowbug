#include "pch.h"
#include "hsx_internals.h"

namespace hsp_sdk_ext {
	auto param_data_to_type(HspParamData const& param_data) -> HspParamType {
		return (HspParamType)param_data.param()->mptype;
	}

	auto param_data_to_pval(HspParamData const& param_data) -> std::optional<PVal const*> {
		auto type = param_data_to_type(param_data);
		if (type != MPTYPE_LOCALVAR) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((PVal const*)param_data.ptr()));
	}

	auto param_data_to_mp_var(HspParamData const& param_data) -> std::optional<MPVarData const*> {
		auto type = param_data_to_type(param_data);
		if (type != MPTYPE_SINGLEVAR && type != MPTYPE_ARRAYVAR) {
			return std::nullopt;
		}
		return std::make_optional(UNSAFE((MPVarData const*)param_data.ptr()));
	}

	auto param_data_to_mp_mod_var(HspParamType type, void const* data)->std::optional<MPModVarData const*> {
		if (type != MPTYPE_MODULEVAR && type != MPTYPE_IMODULEVAR && type != MPTYPE_TMODULEVAR) {
			return std::nullopt;
		}

		auto mp_mod_var = UNSAFE((MPModVarData const*)data);
		if (!mp_mod_var || mp_mod_var->magic != MODVAR_MAGICCODE || !mp_mod_var->pval || mp_mod_var->aptr < 0) {
			return std::nullopt;
		}

		return std::make_optional(mp_mod_var);
	}

	auto param_data_to_mp_mod_var(HspParamData const& param_data) -> std::optional<MPModVarData const*> {
		auto type = param_data_to_type(param_data);
		auto data = param_data.ptr();
		return param_data_to_mp_mod_var(type, data);
	}

	auto param_data_to_data(HspParamData const& param_data) -> std::optional<HspData> {
		if (!param_data.safety()) {
			return std::nullopt;
		}

		switch (param_data_to_type(param_data)) {
		case MPTYPE_LABEL: {
			auto ptr = UNSAFE((HspLabel const*)param_data.ptr());
			return std::make_optional(data_from_label(ptr));
		}
		case MPTYPE_LOCALSTRING: {
			auto str = UNSAFE(*(char const**)param_data.ptr());
			if (!str) {
				assert(false && u8"str param must not be null");
				return std::nullopt;
			}
			return std::make_optional(data_from_str(str));
		}
		case MPTYPE_DNUM: {
			auto ptr = UNSAFE((HspDouble const*)param_data.ptr());
			return std::make_optional(data_from_double(ptr));
		}
		case MPTYPE_INUM: {
			auto ptr = UNSAFE((HspInt const*)param_data.ptr());
			return std::make_optional(data_from_int(ptr));
		}
		default:
			return std::nullopt;
		}
	}
}
