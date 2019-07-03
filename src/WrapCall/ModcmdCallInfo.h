// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H
#ifdef with_WrapCall

#include "../main.h"

namespace WrapCall
{

// 呼び出し直前の情報
struct ModcmdCallInfo
{
	using shared_ptr_type = std::shared_ptr<ModcmdCallInfo const>;
	using weak_ptr_type = std::weak_ptr<ModcmdCallInfo const>;

	std::size_t call_frame_id_;

	// 呼び出されたコマンド
	stdat_t const stdat;

	// 呼び出し直前での prmstk
	void* const prevPrmstk;

	// 呼び出し直前でのネストレベル
	int const sublev;
	int const looplev;

	// 呼び出し側の位置
	hpiutil::SourcePos const callerPos;

	// g_stkCallInfo における位置
	size_t const idx;

public:
	ModcmdCallInfo(std::size_t call_frame_id, stdat_t stdat, void* prevPrmstk, int sublev, int looplev, hpiutil::SourcePos const& callerPos, size_t idx)
		: call_frame_id_(call_frame_id), stdat(stdat), prevPrmstk(prevPrmstk), sublev(sublev), looplev(looplev), callerPos(callerPos), idx(idx)
	{ }

	auto call_frame_id() const -> std::size_t {
		return call_frame_id_;
	}

	auto tryGetPrev() const -> shared_ptr_type;
	auto tryGetNext() const -> shared_ptr_type;

	//prmstk: この呼び出しの実引数情報。
	//safety: このprmstkは確実に正しいものであるか。
	auto tryGetPrmstk() const -> std::pair<void*, bool>;

	// この呼び出しを実引数式に含む呼び出し
	auto tryGetDependedCallInfo() const -> shared_ptr_type;

	auto name() const -> string { return hpiutil::STRUCTDAT_name(stdat); }
};

} //namespace WrapCall

#endif //defined(with_WrapCall)
#endif
