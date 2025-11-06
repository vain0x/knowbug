//! Knowbug の本体

#pragma once

class HspObjectPath;
class HspObjects;
class StepControl;

class KnowbugApp {
public:
	static auto instance() -> std::shared_ptr<KnowbugApp>;

	virtual ~KnowbugApp() {
	}

	virtual void will_exit() = 0;

	virtual auto objects()->HspObjects & = 0;
};
