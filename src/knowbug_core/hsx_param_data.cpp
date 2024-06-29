#include "pch.h"
#include "hsx_internals.h"

namespace hsx {
	auto mp_var_to_pval(MPVarData const* mp_var) -> PVal const* {
		assert(mp_var != nullptr);
		assert(mp_var->pval != nullptr);
		return mp_var->pval;
	}

	auto mp_var_to_aptr(MPVarData const* mp_var) -> std::size_t {
		assert(mp_var != nullptr);
		assert(mp_var->pval != nullptr);
		assert(mp_var->aptr >= 0);
		return (std::size_t)mp_var->aptr;
	}

	auto mp_mod_var_to_pval(MPModVarData const* mp_mod_var) -> PVal const* {
		assert(mp_mod_var != nullptr);
		assert(mp_mod_var->magic == MODVAR_MAGICCODE);
		assert(mp_mod_var->pval != nullptr);
		return mp_mod_var->pval;
	}

	auto mp_mod_var_to_aptr(MPModVarData const* mp_mod_var) -> std::size_t {
		assert(mp_mod_var != nullptr);
		assert(mp_mod_var->magic == MODVAR_MAGICCODE);
		assert(mp_mod_var->aptr >= 0);
		return (std::size_t)mp_mod_var->aptr;
	}

	auto param_data_to_type(HspParamData const& param_data) -> HsxMptype {
		return param_data.param()->mptype;
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

	auto param_data_to_mp_mod_var(HsxMptype type, void const* data)->std::optional<MPModVarData const*> {
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
			auto ptr = UNSAFE((HsxLabel const*)param_data.ptr());
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
			auto ptr = UNSAFE((HsxDouble const*)param_data.ptr());
			return std::make_optional(data_from_double(ptr));
		}
		case MPTYPE_INUM: {
			auto ptr = UNSAFE((HsxInt const*)param_data.ptr());
			return std::make_optional(data_from_int(ptr));
		}
		default:
			return std::nullopt;
		}
	}

	auto param_data_to_str(HspParamData const& param_data) -> std::optional<HsxStrSpan> {
		if (!param_data.safety()) {
			return std::nullopt;
		}

		switch (param_data_to_type(param_data)) {
		case MPTYPE_LOCALSTRING: {
			auto str = UNSAFE(*(char const**)param_data.ptr());
			if (!str) {
				assert(false && u8"str param must not be null");
				return std::nullopt;
			}

			// NOTE: str 引数の値は必ず NULL 終端されている。hsp3_code.cpp/code_expandstruct を参照。
			// FIXME: 値は不変なので、文字数はキャッシュできる。キャッシュを削除するタイミングが微妙。
			auto size = std::strlen(str) + 1;

			return std::make_optional(HsxStrSpan{ str, size });
		}
		default:
			return std::nullopt;
		}
	}
}
