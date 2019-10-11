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

static constexpr auto WM_KNOWBUG_DID_INITIALIZE = WM_USER + 1;
static constexpr auto WM_KNOWBUG_UPDATE_VIEW = WM_USER + 2;

class KnowbugView {
public:
	static auto create(KnowbugConfig const& config, HINSTANCE instance, HspObjects& objects, HspObjectTree& object_tree) -> std::unique_ptr<KnowbugView>;

	virtual ~KnowbugView() {
	}

	virtual void initialize() = 0;

	virtual void update() = 0;

	virtual void update_source_edit(OsStringView const& content) = 0;

	virtual void did_log_change() = 0;

	virtual void will_exit() = 0;

	virtual auto select_save_log_file() -> std::optional<OsString> = 0;

	virtual void notify_save_failure() = 0;

	virtual void notify_open_file_failure() = 0;

	virtual auto process_main_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;

	virtual auto process_view_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;
};
