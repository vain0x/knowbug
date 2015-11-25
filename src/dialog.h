// knowbug dialogs

#pragma once

#include "main.h"
#include <Windows.h>
#include <CommCtrl.h>

namespace Dialog
{

HWND getVarTreeHandle();

void createMain();
void destroyMain();

void update();
bool logsCalling();
optional_ref<string const> tryGetCurrentScript();

namespace View {

void setText(char const* text);
void scroll(int y, int x);
void scrollBottom();
void selectLine(size_t index);
void update();

} // namespace View

} // namespace Dialog
