// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H

#include "WrapCall.h"

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

	//prmstk: この呼び出しの実引数情報。
	//safety: このprmstkは確実に正しいものであるか。
	std::pair<void*, bool> tryGetPrmstk() const;

	// この呼び出しが直接依存されている呼び出しを得る
	optional_ref<ModcmdCallInfo const> tryGetDependedCallInfo() const;
};

} //namespace WrapCall

#endif
