//! knowbug の UI 関連

#pragma once

#include <optional>
#include <Windows.h>
#include "encoding.h"

class HspObjects;
class HspObjectTree;
class HspStaticVars;

namespace hpiutil {
	class DInfo;
}

namespace Dialog
{

void createMain(HspObjects& objects, HspObjectTree& object_tree);
void destroyMain();

void update();

void update_source_view(OsStringView const& content);

void did_log_change();

auto select_save_log_file() -> std::optional<OsString>;

void notify_save_failure();

} // namespace Dialog
