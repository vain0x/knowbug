#include "pch.h"
#include <cassert>
#include "hsx.h"
#include "step_controller.h"

static auto step_mode_to_debug_mode(StepMode mode) -> int {
	switch (mode) {
	case StepMode::Run:
		return HSPDEBUG_RUN;

	case StepMode::Stop:
		return HSPDEBUG_STOP;

	case StepMode::StepIn:
		return HSPDEBUG_STEPIN;

	default:
		assert(false && u8"Unknown StepMode");
		throw new std::exception{};
	}
}

KnowbugStepController::KnowbugStepController(HSP3DEBUG* debug)
	: debug_(debug)
	, step_controller_()
{
}

bool KnowbugStepController::continue_step_running() {
	begin_update();
	auto do_continue = step_controller_.continue_step_running();
	end_update();

	return do_continue;
}

void KnowbugStepController::update(StepControl step_control) {
	begin_update();

	switch (step_control.kind()) {
	case StepControlKind::Run:
		step_controller_.do_run();
		break;

	case StepControlKind::Stop:
		step_controller_.do_stop();
		break;

	case StepControlKind::StepIn:
		step_controller_.do_step_in();
		break;

	case StepControlKind::StepOver:
		step_controller_.do_step_over();
		break;

	case StepControlKind::StepOut:
		step_controller_.do_step_out();
		break;

	case StepControlKind::StepReturn:
		step_controller_.do_step_return(step_control.sublev());
		break;

	default:
		assert(false && u8"Unknown StepControlKind");
		throw new std::exception{};
	}

	end_update();
}

void KnowbugStepController::begin_update() {
	step_controller_.update(ctx->sublev);
}

void KnowbugStepController::end_update() {
	if (step_controller_.is_mode_changed()) {
		hsx::debug_do_set_mode(step_mode_to_debug_mode(step_controller_.mode()), debug_);
	}
}
