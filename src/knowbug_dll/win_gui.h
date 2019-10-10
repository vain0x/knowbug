
#pragma once

#include <string>
#include <memory>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"

using std::string;

auto Window_Create
	( OsStringView className, WNDPROC proc
	, OsStringView caption, int windowStyles
	, int sizeX, int sizeY, int posX, int posY
	, HINSTANCE hInst
	) -> HWND;

void Window_SetTopMost(HWND hwnd, bool isTopMost);

void Menu_ToggleCheck(HMENU menu, UINT itemId, bool& checked);

void Edit_SetTabLength(HWND hEdit, const int tabwidth);

void TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem);
auto TreeView_GetItemAtPoint(HWND hwndTree, POINT pt) -> HTREEITEM;

auto Dialog_SaveFileName(
	HWND owner, LPCTSTR filter, LPCTSTR defaultFilter, LPCTSTR defaultFileName
) -> std::unique_ptr<OsString>;

auto Font_Create(OsStringView family, int size, bool antialias) -> HFONT;
