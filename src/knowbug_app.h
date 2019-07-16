//! Knowbug の本体

#pragma once

class HspObjectPath;
class HspRuntime;
class KnowbugView;
class StepControl;

class KnowbugApp {
public:
	static auto instance() -> std::shared_ptr<KnowbugApp>;

	virtual ~KnowbugApp() {
	}

	virtual auto view() -> KnowbugView& = 0;

	virtual void step_run(StepControl const& step_control) = 0;

	virtual void add_object_text_to_log(HspObjectPath const& path) = 0;

	virtual void clear_log() = 0;

	virtual void save_log() = 0;

	virtual void open_current_script_file() = 0;

	virtual void open_config_file() = 0;

	virtual void open_knowbug_repository() = 0;
};
