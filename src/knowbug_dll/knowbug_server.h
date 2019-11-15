#pragma once

#include <memory>
#include <thread>
#include "../hspsdk/hsp3debug.h"
#include "../knowbug_core/platform.h"

class KnowbugServer {
public:
	static auto create(HSP3DEBUG* debug, HINSTANCE instance)->std::shared_ptr<KnowbugServer>;

	virtual ~KnowbugServer() {
	}

	virtual void start() = 0;

	virtual void will_exit() = 0;
};
