//! ステップ実行関連

#pragma once

struct HSP3DEBUG;
class KnowbugStepController;

// 実行モードを表す。HSPDEBUG_RUN_* と同じ。
enum class StepMode {
	Run,
	Stop,
	StepIn,
};

// ステップ実行の操作の種類を表す。
enum class StepControlKind {
	// (実行)
	// 実行を再開する。
	Run,

	// (停止)
	// 実行を中断して停止する。
	Stop,

	// (次へ)
	// 次の命令を実行して、再び停止する。停止中のみ有効。
	StepIn,

	// (次飛)
	// 次の命令を実行して、再び停止する。
	// 次の命令がユーザー定義命令やサブルーチンなら、それが完了するまで実行してから停止する。
	StepOver,

	// (脱出)
	// 実行中のユーザー定義命令やサブルーチンから return するまで実行して、停止する。
	StepOut,

	// 「この呼び出しから脱出する」
	// 指定したユーザー定義命令から return するまで実行して、停止する。
	StepReturn,
};

// ステップ実行の操作を表す。
class StepControl {
	StepControlKind kind_;
	int sublev_;

	StepControl(StepControlKind kind, int sublev)
		: kind_(kind)
		, sublev_(sublev)
	{
	}

public:
	static auto new_run() -> StepControl {
		return StepControl{ StepControlKind::Run, 0 };
	}

	static auto new_stop() -> StepControl {
		return StepControl{ StepControlKind::Stop, 0 };
	}

	static auto new_step_in() -> StepControl {
		return StepControl{ StepControlKind::StepIn, 0 };
	}

	static auto new_step_over() -> StepControl {
		return StepControl{ StepControlKind::StepOver, 0 };
	}

	static auto new_step_out() -> StepControl {
		return StepControl{ StepControlKind::StepOut, 0 };
	}

	static auto new_step_return(int sublev) -> StepControl {
		return StepControl{ StepControlKind::StepReturn, sublev };
	}

	auto kind() const -> StepControlKind {
		return kind_;
	}

	auto sublev() const -> int {
		return sublev_;
	}
};

// ステップ操作を受け取ってステップ実行モードを制御する機能を担当する。
class StepController {
	// 条件付き実行の終了条件となる sublev
	// (条件付き実行ではないときは -1)
	int sublevOfGoal;

	// 現在のサブルーチンレベル
	int sublev;

	// 現在の実行モード
	StepMode mode_;
	bool mode_dirty_;

public:
	StepController()
		: sublevOfGoal(-1)
		, sublev(0)
		, mode_(StepMode::Run)
		, mode_dirty_(false)
	{
	}

	void update(int sublev) {
		this->sublev = sublev;
		mode_dirty_ = false;
	}

	auto mode() const -> StepMode {
		return mode_;
	}

	auto is_mode_changed() const -> bool {
		return mode_dirty_;
	}

	void runStop() {
		setStepMode(StepMode::Stop);
	}

	void run() {
		setStepMode(StepMode::Run);
	}

	void runStepIn() {
		setStepMode(StepMode::StepIn);
	}

	void runStepOver() {
		return runStepReturn(sublev);
	}

	void runStepOut() {
		return runStepReturn(sublev - 1);
	}

	// ctx->sublev == sublev になるまで step を繰り返す
	void runStepReturn(int sublev) {
		if (sublev < 0) return run();

		sublevOfGoal = sublev;
		setStepMode(StepMode::StepIn);
	}

	// 条件付き実行が継続されるか？
	bool continueConditionalRun() {
		if (sublevOfGoal >= 0) {
			if (sublev > sublevOfGoal) {
				setStepMode(StepMode::StepIn); // stepin を繰り返す
				return true;
			}
			else {
				sublevOfGoal = -1;
			}
		}
		return false;
	}

private:
	void setStepMode(StepMode mode) {
		mode_ = mode;
		mode_dirty_ = true;
	}
};

class KnowbugStepController {
	HSP3DEBUG* debug_;

	StepController step_controller_;

public:
	explicit KnowbugStepController(HSP3DEBUG* debug);

	bool continue_step_running();

	void update(StepControl step_control);

private:
	void begin_update();
	void end_update();
};
