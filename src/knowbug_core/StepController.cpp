#include <cassert>
#include "../hpiutil/hpiutil.hpp"
#include "DebugInfo.h"
#include "StepController.h"

static auto step_mode_to_hsp_run_mode(StepMode mode) -> int {
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

KnowbugStepController::KnowbugStepController(HSPCTX const* hspctx, DebugInfo& debug_info)
	: hspctx_(hspctx_)
	, debug_info_(debug_info)
	, step_controller_()
{
}

bool KnowbugStepController::continue_step_running() {
	begin_update();
	auto do_continue = step_controller_.continueConditionalRun();
	end_update();

	return do_continue;
}

void KnowbugStepController::update(StepControl step_control) {
	begin_update();

	switch (step_control.kind()) {
	case StepControlKind::Run:
		step_controller_.run();
		break;

	case StepControlKind::Stop:
		step_controller_.runStop();
		break;

	case StepControlKind::StepIn:
		step_controller_.runStepIn();
		break;

	case StepControlKind::StepOver:
		step_controller_.runStepOver();
		break;

	case StepControlKind::StepOut:
		step_controller_.runStepOut();
		break;

	case StepControlKind::StepReturn:
		step_controller_.runStepReturn(step_control.sublev());
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
		debug_info_.setStepMode(step_mode_to_hsp_run_mode(step_controller_.mode()));
	}
}
