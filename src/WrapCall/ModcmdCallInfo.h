// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H

#include "../main.h"
#include "WrapCall.h"

namespace WrapCall
{

// 呼び出し直前の情報
// Remark: Don't rearrange the members.
// 全メンバが const な構造体に const をつける意味とは
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
	
	/*
	// 連結リスト
	ModcmdCallInfo const* prev;
	ModcmdCallInfo const* next;
	//*/

public:
	ModcmdCallInfo(stdat_t stdat, void* prmstk_bak, int sublev, int looplev, char const* fname, int line, size_t idx)
		: stdat(stdat), prmstk_bak(prmstk_bak), sublev(sublev), looplev(looplev), fname(fname), line(line), idx(idx)
	{ }

	// 実行中か (引数展開が終了しているか)
	bool isRunning() const {
		return (getNext() == nullptr || sublev < getNext()->sublev);
	}
	
	// result is optional (none: nullptr)
	ModcmdCallInfo const* getPrev() const {
		return getCallInfoAt(idx - 1);
	}
	ModcmdCallInfo const* getNext() const {
		return getCallInfoAt(idx + 1);
	}

	// この呼び出しの実引数情報を持つ prmstack を得る。
	// 存在しないか、取得できなさそうなとき nullptr を返す。
	void* getPrmstk() const
	{
		// 最新の呼び出し
		if ( getNext() == nullptr ) {
			// 引数の中から更に入れ子の呼び出しがある場合、触らない方が無難
			if ( ctx->sublev - sublev >= 2 ) {
				return nullptr;
			} else {
				return ctx->prmstack;
			}

		// 実行中の呼び出し (引数展開が完了している)
		} else if ( isRunning() ) {
			return getNext()->prmstk_bak;

		// 引数展開中 (スタックフレームが未完成なので prmstack は参照できない)
		} else {
			return nullptr;
		}
	}
};

}

#endif
