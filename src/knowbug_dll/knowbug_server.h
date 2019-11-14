#pragma once

#include <memory>
#include <thread>
#include "../hspsdk/hsp3debug.h"
#include "../knowbug_core/platform.h"

class KnowbugServer {
public:
	static auto start(HSP3DEBUG* debug, HINSTANCE instance)->std::shared_ptr<KnowbugServer>;

	virtual ~KnowbugServer() {
	}

	virtual void will_exit() = 0;
};
