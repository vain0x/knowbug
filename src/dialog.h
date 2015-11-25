// knowbug dialogs

#pragma once

#include <Windows.h>

namespace Dialog
{

HWND getVarTreeHandle();

void createMain();
void destroyMain();

void update();
bool logsCalling();

namespace View {

void setText(char const* text);
void scroll(int y, int x);
void scrollBottom();
void selectLine(size_t index);
void update();

} // namespace View

} // namespace Dialog
