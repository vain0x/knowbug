//! WrapCall: ユーザ定義コマンドの呼び出しをフックする仕組み

#pragma once

#include <optional>
#include "../hspsdk/hsp3struct.h"
#include "HspTypes.h"

class WcCallFrame;
class WcDebugger;

// WrapCall を初期化する。デバッガーの起動時に必ず呼び出すこと。
extern void wc_initialize(std::shared_ptr<WcDebugger> const& debugger);

extern auto wc_call_frame_count() -> std::size_t;

extern auto wc_call_frame_id_at(std::size_t index) -> std::optional<std::size_t>;

extern auto wc_call_frame_get(std::size_t call_frame_id) -> std::optional<std::reference_wrapper<WcCallFrame>>;

extern auto wc_call_frame_to_param_stack(std::size_t call_frame_id) -> std::optional<HspParamStack>;

// ユーザ定義コマンドの呼び出し情報
class WcCallFrame {
	std::size_t call_frame_id_;

	// 呼び出されたコマンド
	STRUCTDAT const* struct_dat_;

	// 呼び出し直前での引数スタックのデータ
	void const* prev_param_stack_;

	// 呼び出し直前でのネストレベル
	int prev_sublev_;
	int prev_looplev_;

	// 呼び出し側の位置
	std::optional<std::size_t> file_id_opt_;

	std::size_t line_index_;

public:
	WcCallFrame(
		std::size_t call_frame_id,
		STRUCTDAT const* struct_dat,
		void const* prev_param_stack,
		int prev_sublev,
		int prev_looplev,
		std::optional<std::size_t>&& file_id_opt,
		std::size_t line_index
	)
		: call_frame_id_(call_frame_id)
		, struct_dat_(struct_dat)
		, prev_param_stack_(prev_param_stack)
		, prev_sublev_(prev_sublev)
		, prev_looplev_(prev_looplev)
		, file_id_opt_(std::move(file_id_opt))
		, line_index_(line_index)
	{
	}

	auto call_frame_id() const -> std::size_t {
		return call_frame_id_;
	}

	auto struct_dat() const -> STRUCTDAT const* {
		return struct_dat_;
	}

	auto prev_param_stack() const -> void const* {
		return prev_param_stack_;
	}

	auto prev_sublev() const -> int {
		return prev_sublev_;
	}

	auto prev_looplev() const -> int {
		return prev_looplev_;
	}

	auto file_id_opt() const -> std::optional<std::size_t> {
		return file_id_opt_;
	}

	auto line_index() const -> std::size_t {
		return line_index_;
	}
};

// WrapCall からデバッガーにアクセスするためのもの
class WcDebugger {
public:
	virtual ~WcDebugger() {
	}

	virtual void get_current_location(std::optional<std::size_t>& file_id_opt, std::size_t& line_index) = 0;
};
