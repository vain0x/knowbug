//! knowbug の UI 関連

#pragma once

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

auto getVarTreeHandle() -> HWND;

void createMain(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars, HspObjectTree& object_tree);
void destroyMain();

void update();

void did_log_change();

auto confirm_to_clear_log() -> bool;

namespace View {

void setText(OsStringView const& text);
void scroll(int y, int x);
void scrollBottom();
void selectLine(size_t index);
void update();
void saveCurrentCaret();

} // namespace View

} // namespace Dialog
