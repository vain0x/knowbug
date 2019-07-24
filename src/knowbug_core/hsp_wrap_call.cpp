#include "pch.h"
#include <vector>
#include "../hpiutil/hpiutil.hpp"
#include "hsp_wrap_call.h"
#include "DebugInfo.h"
#include "platform.h"

static auto s_debugger = std::weak_ptr<WcDebugger>{};

static auto s_last_id = std::size_t{};

static auto s_call_stack = std::vector<WcCallFrame>{};

static auto s_modcmd_cmdfunc_impl = static_cast<decltype(HSP3TYPEINFO::cmdfunc)>(nullptr);

static auto s_modcmd_reffunc_impl = static_cast<decltype(HSP3TYPEINFO::reffunc)>(nullptr);

static auto modcmd_cmdfunc(int cmdid) -> int;

static auto modcmd_reffunc(int* type_res, int cmdid) -> void*;

void wc_initialize(std::shared_ptr<WcDebugger> const& debugger) {
	s_debugger = debugger;
}

// ユーザ定義コマンドの呼び出し直前に呼ばれる
static void wc_will_call(STRUCTDAT const* struct_dat) {
	auto debugger = s_debugger.lock();
	if (!debugger) {
		return;
	}

	std::string file_ref_name;
	std::size_t line_index;
	debugger->get_current_location(file_ref_name, line_index);

	s_call_stack.emplace_back(
		++s_last_id,
		struct_dat,
		ctx->prmstack,
		ctx->sublev,
		ctx->looplev,
		std::move(file_ref_name),
		line_index
	);
}

// ユーザ定義命令の呼び出し直後に呼ばれる
static void wc_did_call(PDAT* p, int vt) {
	// FIXME: 警告表示機能を戻す
	// // 警告
	// if ( ctx->looplev != callinfo->looplev ) {
	// 	Knowbug::logmesWarning(u8"呼び出し中に入った loop から正常に脱出しないまま、呼び出しが終了した。");
	// }

	// if ( ctx->sublev != callinfo->sublev ) {
	// 	Knowbug::logmesWarning(u8"呼び出し中に入ったサブルーチンから正常に脱出しないまま、呼び出しが終了した。");
	// }

	if (!s_call_stack.empty()) {
		s_call_stack.pop_back();
	}
}

void wc_did_call() {
	return wc_did_call(nullptr, HSPVAR_FLAG_NONE);
}

auto wc_call_frame_count() -> std::size_t {
	return s_call_stack.size();
}

auto wc_call_frame_id_at(std::size_t index) -> std::optional<std::size_t> {
	if (index >= s_call_stack.size()) {
		return std::nullopt;
	}

	return std::make_optional(s_call_stack.at(index).call_frame_id());
}

static auto wc_call_frame_position(std::size_t call_frame_id) -> std::optional<std::size_t> {
	for (auto i = std::size_t{}; i < s_call_stack.size(); i++) {
		if (s_call_stack[i].call_frame_id() == call_frame_id) {
			return std::make_optional(i);
		}
	}

	return std::nullopt;
}

auto wc_call_frame_get(std::size_t call_frame_id) -> std::optional<std::reference_wrapper<WcCallFrame>> {
	auto index_opt = wc_call_frame_position(call_frame_id);
	if (!index_opt) {
		return std::nullopt;
	}

	return std::make_optional(std::ref(s_call_stack.at(*index_opt)));
}

// コールフレームの引数スタックを取得する。
auto wc_call_frame_to_param_stack(std::size_t call_frame_id) -> std::optional<HspParamStack> {
	// 注意: 必ずしもすべての呼び出しをフックできているとは限らない点に注意。
	//
	// 1. 例えば modinit の呼び出しはコールスタックに乗らない。
	// 仮に f → modinit → g の順で呼ばれているとすると、
	// f の次のフレームは g になっているが、g の呼び出し直前の引数スタックは f のものではない。
	//
	// 2. また、f(g()) のような形の式では、f → g の順でスタックに積まれて、
	// f の引数スタックは g() が完了するまで参照できない。

	auto index_opt = wc_call_frame_position(call_frame_id);
	if (!index_opt) {
		return std::nullopt;
	}

	auto index = *index_opt;

	auto is_last = index + 1 == s_call_stack.size();
	auto next_param_stack = is_last
		? ctx->prmstack
		: s_call_stack[index + 1].prev_param_stack();
	auto next_sublev = is_last
		? ctx->sublev
		: s_call_stack[index + 1].prev_sublev();

	auto struct_dat = s_call_stack[index].struct_dat();
	auto prev_sublev = s_call_stack[index].prev_sublev();

	// 引数スタックが存在するための条件
	auto exists = next_sublev > prev_sublev;

	// 引数スタックが真正であるための条件
	auto is_safe = next_sublev == prev_sublev + 1;

	auto param_stack = exists ? next_param_stack : nullptr;

	// FIXME: DebugApi を使う?
	auto param_stack_size = struct_dat->size;

	return std::make_optional<HspParamStack>(struct_dat, (void*)param_stack, param_stack_size, is_safe);
}

// ユーザ定義コマンドの処理をラッパーで置き換える
static void modcmd_init(HSP3TYPEINFO* info) {
	if (s_modcmd_cmdfunc_impl) {
		return;
	}

	s_modcmd_cmdfunc_impl = info->cmdfunc;
	s_modcmd_reffunc_impl = info->reffunc;

	info->cmdfunc = modcmd_cmdfunc;
	info->reffunc = modcmd_reffunc;
}

// ユーザ定義命令の呼び出し処理のラッパー
static auto modcmd_cmdfunc(int cmdid) -> int {
	auto struct_dat = &hpiutil::finfo()[cmdid];

	wc_will_call(struct_dat);
	auto runmode = s_modcmd_cmdfunc_impl(cmdid);
	wc_did_call();
	return runmode;
}

// ユーザ定義関数の呼び出し処理のラッパー
static auto modcmd_reffunc(int* type_res, int cmdid) -> void* {
	auto struct_dat = &hpiutil::finfo()[cmdid];

	wc_will_call(struct_dat);
	auto result = s_modcmd_reffunc_impl(type_res, cmdid);
	wc_did_call((PDAT*)result, *type_res);
	return result;
}

// プラグイン初期化関数
EXPORT void WINAPI hsp3hpi_init_wrapcall(HSP3TYPEINFO* info) {
	hsp3sdk_init(info);

	// 初期化 (HSP ランタイムの実装に依存している)
	auto const typeinfo = &info[- info->type];
	modcmd_init(&typeinfo[TYPE_MODCMD]);

	s_call_stack.reserve(128);
}
