#pragma once

#include <memory>
#include <thread>
#include "../hspsdk/hsp3debug.h"
#include "../knowbug_core/platform.h"

class HspObjects;

class KnowbugServer {
public:
	static auto create(HSP3DEBUG* debug, HspObjects& objects, HINSTANCE instance, KnowbugStepController& step_controller)->std::shared_ptr<KnowbugServer>;

	virtual ~KnowbugServer() {
	}

	virtual void start() = 0;

	virtual void will_exit() = 0;

	virtual void logmes(HspStringView text) = 0;

	virtual void debuggee_did_stop() = 0;
};
