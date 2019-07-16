
#pragma once

#include <string>
#include <memory>
#include "../encoding.h"
#include "../platform.h"

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
void Edit_UpdateText(HWND hEdit, char const* s);
void Edit_SetSelLast(HWND hEdit);

auto TreeView_GetItemString(HWND hwndTree, HTREEITEM hItem) -> string;
auto TreeView_GetItemLParam(HWND hwndTree, HTREEITEM hItem) -> LPARAM;
void   TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem);
auto TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem) -> HTREEITEM;
auto TreeView_GetItemAtPoint(HWND hwndTree, POINT pt) -> HTREEITEM;

auto Dialog_SaveFileName(
	HWND owner, LPCTSTR filter, LPCTSTR defaultFilter, LPCTSTR defaultFileName
) -> std::unique_ptr<OsString>;

auto Font_Create(OsStringView family, int size, bool antialias) -> HFONT;
