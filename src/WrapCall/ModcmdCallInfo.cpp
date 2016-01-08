#ifdef with_WrapCall

#include "WrapCall.h"
#include "ModcmdCallInfo.h"

//unhookable invocation …… modinit/modterm/deffunc_onexit commands, and call's by call.hpi
// これらの呼び出しはprmstkを変更するにもかかわらず検知できない。
// prmstk を異なるシグネチャで読み取ると不正な値をポインタとみなすことになり、危険である。

namespace WrapCall
{

auto ModcmdCallInfo::tryGetPrev() const -> ModcmdCallInfo::shared_ptr_type
{
	return tryGetCallInfoAt(idx - 1);
}

auto ModcmdCallInfo::tryGetNext() const -> ModcmdCallInfo::shared_ptr_type
{
	return tryGetCallInfoAt(idx + 1);
}

auto ModcmdCallInfo::tryGetPrmstk() const -> std::pair<void*, bool>
{
	auto optNext = tryGetNext();

	//これが最新の呼び出し
	if ( ! optNext ) {
		assert(sublev <= ctx->sublev);
		//本体からさらに他のサブルーチンが実行中なら、それはunhookable invocationの可能性がある
		auto const safe =
			ctx->sublev == sublev + 1;

		return { ctx->prmstack, safe };

	//呼び出しが実行中
	//⇔ 次の呼び出しがあり、それはこれの実引数式からの呼び出しではない
	//⇔ 次の呼び出しがあり、それはこれの本体(またはそれより深い位置)から呼び出されている
	} else if ( sublev < optNext->sublev ) {
		assert(sublev + 1 <= optNext->sublev);
		//本体からさらに他のサブルーチンが実行中なら、それはunhookable invocationの可能性がある
		auto const safe =
			sublev + 1 == optNext->sublev;

		return { optNext->prevPrmstk, safe };

	// 引数展開中
	//⇔prmstack は未作成
	} else {
		return { nullptr, false };
	}
}

// この呼び出しが直接依存されている呼び出しを得る。(failure: nullptr)
auto ModcmdCallInfo::tryGetDependedCallInfo() const -> ModcmdCallInfo::shared_ptr_type
{
	auto optPrev = tryGetPrev();
	return (optPrev && optPrev->sublev == sublev)
		? optPrev
		: nullptr;
}

} //namespace WrapCall

#endif //defined(with_WrapCall)
