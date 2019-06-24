// knowbug dialogs

#pragma once

#include <Windows.h>

class HspObjects;
class HspObjectTree;
class HspStaticVars;
class OsStringView;

namespace hpiutil {
	class DInfo;
}

namespace Dialog
{

auto getVarTreeHandle() -> HWND;

void createMain(hpiutil::DInfo const& debug_segment, HspObjects& objects, HspStaticVars& static_vars, HspObjectTree& object_tree);
void destroyMain();

void update();
bool logsCalling();

namespace View {

void setText(OsStringView const& text);
void scroll(int y, int x);
void scrollBottom();
void selectLine(size_t index);
void update();
void saveCurrentCaret();

} // namespace View

} // namespace Dialog
