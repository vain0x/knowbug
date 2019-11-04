//! knowbug の UI 関連

#pragma once

#include <optional>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_tree.h"
#include "../knowbug_core/platform.h"

class HspObjects;
class HspObjectTree;
class KnowbugApp;
class KnowbugConfig;

static constexpr auto WM_KNOWBUG_DID_INITIALIZE = WM_USER + 1;
static constexpr auto WM_KNOWBUG_UPDATE_VIEW = WM_USER + 2;
static constexpr auto WM_KNOWBUG_DID_UPDATE = WM_USER + 3;

class KnowbugView {
public:
	static auto create(KnowbugConfig const& config, HINSTANCE instance, HspObjectTree& object_tree) -> std::unique_ptr<KnowbugView>;

	virtual ~KnowbugView() {
	}

	virtual auto current_node_id_opt() const->std::optional<std::size_t> = 0;

	virtual void initialize() = 0;

	virtual void update(HspObjects& objects, HspObjectTree& object_tree) = 0;

	virtual void update_source_edit(OsStringView const& content) = 0;

	virtual void object_node_did_create(std::size_t node_id, HspObjectTreeInsertMode mode, HspObjects& objects, HspObjectTree& object_tree) = 0;

	virtual void object_node_will_destroy(std::size_t node_id, HspObjectTree& object_tree) = 0;

	virtual void will_exit() = 0;

	virtual auto select_save_log_file() -> std::optional<OsString> = 0;

	virtual void notify_save_failure() = 0;

	virtual void notify_open_file_failure() = 0;

	virtual auto process_main_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;

	virtual auto process_view_window(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, KnowbugApp& app) -> LRESULT = 0;
};
