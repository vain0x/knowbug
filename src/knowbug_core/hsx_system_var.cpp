#include "pch.h"
#include "hsx_internals.h"

#undef stat

namespace hsx {
	auto system_var_cnt(HSPCTX const* ctx) -> std::optional<HsxInt const*> {
		auto looplev = *system_var_looplev(ctx);
		// FIXME: looplev=0 のとき nullopt?
		return std::make_optional(&ctx->mem_loop[looplev].cnt);
	}

	auto system_var_err(HSPCTX const* ctx) -> HsxInt const* {
		static_assert(sizeof(ctx->err) == sizeof(HsxInt), u8"HSPERROR は int のはず");
		return UNSAFE((HsxInt const*)(&ctx->err));
	}

	auto system_var_iparam(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->iparam;
	}

	auto system_var_wparam(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->wparam;
	}

	auto system_var_lparam(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->lparam;
	}

	auto system_var_looplev(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->looplev;
	}

	auto system_var_sublev(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->sublev;
	}

	auto system_var_refstr(HSPCTX const* ctx) -> HsxStrSpan {
		return HsxStrSpan{ ctx->refstr, HSPCTX_REFSTR_MAX };
	}

	auto system_var_refdval(HSPCTX const* ctx) -> HsxDouble const* {
		return &ctx->refdval;
	}

	auto system_var_stat(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->stat;
	}

	auto system_var_strsize(HSPCTX const* ctx) -> HsxInt const* {
		return &ctx->strsize;
	}

	auto system_var_thismod(HSPCTX const* ctx) -> std::optional<MPModVarData const*> {
		return param_data_to_mp_mod_var(MPTYPE_MODULEVAR, ctx->prmstack);
	}

	auto system_var_to_data(HsxSysvarKind system_var_kind, HSPCTX const* ctx) -> std::optional<HsxData> {
		switch (system_var_kind) {
		case HSX_SYSVAR_CNT:
			return data_from_int_opt(system_var_cnt(ctx));

		case HSX_SYSVAR_ERR:
			return data_from_int(system_var_err(ctx));

		case HSX_SYSVAR_IPARAM:
			return data_from_int(system_var_iparam(ctx));

		case HSX_SYSVAR_WPARAM:
			return data_from_int(system_var_wparam(ctx));

		case HSX_SYSVAR_LPARAM:
			return data_from_int(system_var_lparam(ctx));

		case HSX_SYSVAR_LOOPLEV:
			return data_from_int(system_var_looplev(ctx));

		case HSX_SYSVAR_SUBLEV:
			return data_from_int(system_var_sublev(ctx));

		case HSX_SYSVAR_REFSTR:
			return data_from_str(system_var_refstr(ctx).data);

		case HSX_SYSVAR_REFDVAL:
			return data_from_double(system_var_refdval(ctx));

		case HSX_SYSVAR_STAT:
			return data_from_int(system_var_stat(ctx));

		case HSX_SYSVAR_STRSIZE:
			return data_from_int(system_var_strsize(ctx));

		case HSX_SYSVAR_THISMOD: {
			auto mp_mod_var_opt = system_var_thismod(ctx);
			if (!mp_mod_var_opt) {
				return std::nullopt;
			}

			auto pval = mp_mod_var_to_pval(*mp_mod_var_opt);
			auto aptr = mp_mod_var_to_aptr(*mp_mod_var_opt);
			return element_to_data(pval, aptr, ctx);
		}
		default:
			assert(false && u8"Invalid HsxSysvarKind");
			return std::nullopt;
		}
	}
}
