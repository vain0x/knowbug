//! knowbug の UI 関連

#pragma once

#include <optional>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"

class HspObjects;
class HspObjectTree;
class HspStaticVars;
class KnowbugApp;
struct KnowbugConfig;

class KnowbugView {
public:
	static auto create(KnowbugConfig const& config, HINSTANCE instance, HspObjects& objects, HspObjectTree& object_tree) -> std::unique_ptr<KnowbugView>;

	virtual ~KnowbugView() {
	}

	virtual void initialize() = 0;

	virtual void update() = 0;

	virtual void update_source_edit(OsStringView const& content) = 0;

	virtual void did_log_change() = 0;

	virtual auto select_save_log_file() -> std::optional<OsString> = 0;

	virtual void notify_save_failure() = 0;

	virtual auto process_main_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;

	virtual auto process_view_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;
};
