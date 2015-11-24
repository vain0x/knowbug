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

} // namespace Dialog
