#include "pch.h"
#include "hsx_internals.h"

#undef stat

namespace hsx {
	auto system_var_cnt(HSPCTX const* ctx) -> std::optional<HspInt const*> {
		auto looplev = *system_var_looplev(ctx);
		// FIXME: looplev=0 のとき nullopt?
		return std::make_optional(&ctx->mem_loop[looplev].cnt);
	}

	auto system_var_err(HSPCTX const* ctx) -> HspInt const* {
		static_assert(sizeof(ctx->err) == sizeof(HspInt), u8"HSPERROR は int のはず");
		return UNSAFE((HspInt const*)(&ctx->err));
	}

	auto system_var_iparam(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->iparam;
	}

	auto system_var_wparam(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->wparam;
	}

	auto system_var_lparam(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->lparam;
	}

	auto system_var_looplev(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->looplev;
	}

	auto system_var_sublev(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->sublev;
	}

	auto system_var_refstr(HSPCTX const* ctx) -> Slice<char> {
		return Slice<char>{ ctx->refstr, HSPCTX_REFSTR_MAX };
	}

	auto system_var_refdval(HSPCTX const* ctx) -> HspDouble const* {
		return &ctx->refdval;
	}

	auto system_var_stat(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->stat;
	}

	auto system_var_strsize(HSPCTX const* ctx) -> HspInt const* {
		return &ctx->strsize;
	}

	auto system_var_thismod(HSPCTX const* ctx) -> std::optional<MPModVarData const*> {
		return param_data_to_mp_mod_var(MPTYPE_MODULEVAR, ctx->prmstack);
	}

	auto system_var_to_data(HspSystemVarKind system_var_kind, HSPCTX const* ctx) -> std::optional<HspData> {
		switch (system_var_kind) {
		case HspSystemVarKind::Cnt:
			return data_from_int_opt(system_var_cnt(ctx));

		case HspSystemVarKind::Err:
			return data_from_int(system_var_err(ctx));

		case HspSystemVarKind::IParam:
			return data_from_int(system_var_iparam(ctx));

		case HspSystemVarKind::WParam:
			return data_from_int(system_var_wparam(ctx));

		case HspSystemVarKind::LParam:
			return data_from_int(system_var_lparam(ctx));

		case HspSystemVarKind::LoopLev:
			return data_from_int(system_var_looplev(ctx));

		case HspSystemVarKind::SubLev:
			return data_from_int(system_var_sublev(ctx));

		case HspSystemVarKind::Refstr:
			return data_from_str(system_var_refstr(ctx).data());

		case HspSystemVarKind::Refdval:
			return data_from_double(system_var_refdval(ctx));

		case HspSystemVarKind::Stat:
			return data_from_int(system_var_stat(ctx));

		case HspSystemVarKind::StrSize:
			return data_from_int(system_var_strsize(ctx));

		case HspSystemVarKind::Thismod: {
			auto mp_mod_var_opt = system_var_thismod(ctx);
			if (!mp_mod_var_opt) {
				return std::nullopt;
			}

			auto pval = mp_mod_var_to_pval(*mp_mod_var_opt);
			auto aptr = mp_mod_var_to_aptr(*mp_mod_var_opt);
			return element_to_data(pval, aptr, ctx);
		}
		default:
			assert(false && u8"Invalid HspSystemVarKind");
			return std::nullopt;
		}
	}
}
