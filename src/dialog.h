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
string const* tryGetCurrentScript();

namespace LogBox
{
	void add(char const* msg);
	string const& get();
}

} // namespace Dialog
