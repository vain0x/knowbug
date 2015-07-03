// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H

//unhookable invocation …… modinit/modterm/deffunc_onexit commands, and call by call.hpi
// これらの呼び出しはprmstkを変更するにもかかわらず検知できないので危険。

#include "../main.h"
#include "WrapCall.h"

#include "../SysvarData.h"

namespace WrapCall
{

// 呼び出し直前の情報
struct ModcmdCallInfo
{
	// 呼び出されたコマンド
	stdat_t const stdat;

	// 呼び出し直前での prmstk
	void* const prmstk_bak;

	// 呼び出し直前でのネストレベル
	int const sublev;
	int const looplev;

	// 呼び出された位置
	char const* const fname;
	int const line; //0-based

	// g_stkCallInfo における位置
	size_t const idx;

public:
	ModcmdCallInfo(stdat_t stdat, void* prmstk_bak, int sublev, int looplev, char const* fname, int line, size_t idx)
		: stdat(stdat), prmstk_bak(prmstk_bak), sublev(sublev), looplev(looplev), fname(fname), line(line), idx(idx)
	{ }

	optional_ref<ModcmdCallInfo const> getPrev() const {
		return getCallInfoAt(idx - 1);
	}
	optional_ref<ModcmdCallInfo const> getNext() const {
		return getCallInfoAt(idx + 1);
	}

	//prmstk: この呼び出しの実引数情報 (failure: nullptr)
	//safety: このprmstkが確実に安全であるか。
	// prmstkはhspのスタックかメモリプール上に確保されるので、メモリアクセスは常に安全。
	std::pair<void*, bool> tryGetPrmstk() const
	{
		//これが最新の呼び出し
		if ( !getNext() ) {
			assert(sublev <= ctx->sublev);
			//本体からさらに他のサブルーチンが実行中なら、それはunhookable invocationの可能性がある
			bool const safe = ( ctx->sublev == sublev + 1 );

			return { ctx->prmstack, safe };

		//呼び出しが実行中
		//⇔ 次の呼び出しがあり、それはこれの実引数式からの呼び出しではない
		//⇔ 次の呼び出しがあり、それはこれの本体(またはそれより深い位置)から呼び出されている
		} else if ( sublev < getNext()->sublev ) {
			assert(sublev + 1 <= getNext()->sublev);
			//本体からさらに他のサブルーチンが実行中なら、それはunhookable invocationの可能性がある
			bool const safe = (sublev + 1 == getNext()->sublev);

			return { getNext()->prmstk_bak, safe };

		// 引数展開中
		//⇔prmstack は未作成
		} else {
			return { nullptr, false };
		}
	}

	// この呼び出しが直接依存されている呼び出しを得る。(failure: nullptr)
	// 条件について：直前の呼び出しで、それと sublev が等しければ依存関係にあり、そうでなければない
	// なお、これも拡張プラグインの gosub を考慮しない。
	optional_ref<ModcmdCallInfo const> getDependedCallInfo() const
	{
		auto const prev = getPrev();
		return (prev && prev->sublev == sublev)
			? prev
			: nullptr;
	}
};

}

#endif
