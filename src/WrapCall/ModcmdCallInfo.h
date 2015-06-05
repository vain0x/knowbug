// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H

#include "../main.h"
#include "WrapCall.h"

#include "../SysvarData.h"

namespace WrapCall
{

// 呼び出し直前の情報
// Remark: Don't rearrange the members.
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
	int const line;

	// g_stkCallInfo における位置
	size_t const idx;

public:
	ModcmdCallInfo(stdat_t stdat, void* prmstk_bak, int sublev, int looplev, char const* fname, int line, size_t idx)
		: stdat(stdat), prmstk_bak(prmstk_bak), sublev(sublev), looplev(looplev), fname(fname), line(line), idx(idx)
	{ }

	// 引数展開が終了しているか
/*
条件について：
1. 他のユーザ定義コマンドが起動されない限り、これの引数展開の間に knowbug は動作しない。
そのため、次のユーザ定義コマンドが存在しなければ、引数展開は終了している。
ただし (call.hpi のような) 拡張プラグインのコマンドにより、ユーザ定義コマンド以外の方法で引数式中から gosub が生じると、
これが最新の呼び出しであるにもかかわらず、引数展開が終了していない状態で knowbug が動く、ということがありうる。
また、modinit による呼び出しの間は ctx->prmstack が変わるため参照できない。
2. 直後のユーザ定義コマンドがこの呼び出しの引数式の中にあるなら、これの引数展開は終了していない。
逆に、直後のユーザ定義コマンドが引数式の中にないなら、この呼び出しの実行は開始されている――すなわち、これの引数展開は終了している。
ただし、1. と同様に、後者は拡張プラグインを考慮していない。
3. 拡張プラグインのコマンドによる式中 gosub を考慮した上で、引数展開が終了しているための十分条件を見つけることはできない。
//*/
	bool isMaybeRunning() const {
		return (!getNext() || sublev < getNext()->sublev);
	}

	// result is optional (none: nullptr)
	ModcmdCallInfo const* getPrev() const {
		return getCallInfoAt(idx - 1);
	}
	ModcmdCallInfo const* getNext() const {
		return getCallInfoAt(idx + 1);
	}

	// この呼び出しの実引数情報を持つ prmstack を得る。(failure: nullptr)
	// なお、ctx->prmstack が勝手に変更されることは考慮していない。
	void* getPrmstk() const
	{
		// 最新の呼び出し
		if ( !getNext() ) {
			// modinit が実行中の可能性があるとき
			if ( ctx->sublev > sublev + 1  && Sysvar::getThismod() ) {
				return nullptr;
			}

			return ctx->prmstack;

		// 引数展開が完了している
		} else if ( isMaybeRunning() ) {
			return getNext()->prmstk_bak;

		// 引数展開中 (そもそも prmstack は未作成)
		} else {
			return nullptr;
		}
	}

	// この呼び出しが直接依存されている呼び出しを得る。(failure: nullptr)
	// 条件について：直前の呼び出しで、それと sublev が等しければ依存関係にあり、そうでなければない
	// なお、これも拡張プラグインの gosub を考慮しない。
	ModcmdCallInfo const* getDependedCallInfo() const
	{
		auto const prev = getPrev();
		return (prev && prev->sublev == sublev)
			? prev
			: nullptr;
	}
};

}

#endif
