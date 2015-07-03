#include "WrapCall.h"
#include "ModcmdCallInfo.h"

//unhookable invocation …… modinit/modterm/deffunc_onexit commands, and call's by call.hpi
// これらの呼び出しはprmstkを変更するにもかかわらず検知できない。
// prmstk を異なるシグネチャで読み取ると不正な値をポインタとみなすことになり、危険である。

namespace WrapCall
{

std::pair<void*, bool> ModcmdCallInfo::tryGetPrmstk() const
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
optional_ref<ModcmdCallInfo const> ModcmdCallInfo::tryGetDependedCallInfo() const
{
	auto const prev = getPrev();
	return (prev && prev->sublev == sublev)
		? prev
		: nullptr;
}

} //namespace WrapCall
