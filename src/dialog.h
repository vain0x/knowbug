// knowbug dialogs

#pragma once

#include <Windows.h>

class OsStringView;

namespace Dialog
{

auto getVarTreeHandle() -> HWND;

void createMain();
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
