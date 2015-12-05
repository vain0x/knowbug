
#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
#include <memory>

using string = std::string;

HWND Window_Create
	( char const* className, WNDPROC proc
	, char const* caption, int windowStyles
	, int sizeX, int sizeY, int posX, int posY
	, HINSTANCE hInst );

void Window_SetTopMost(HWND hwnd, bool isTopMost);

void Menu_ToggleCheck(HMENU menu, UINT itemId, bool& checked);

void Edit_SetTabLength(HWND hEdit, const int tabwidth);
void Edit_UpdateText(HWND hEdit, char const* s);
void Edit_SetSelLast(HWND hEdit);

string TreeView_GetItemString(HWND hwndTree, HTREEITEM hItem);
LPARAM TreeView_GetItemLParam(HWND hwndTree, HTREEITEM hItem);
void   TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem);
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem);
HTREEITEM TreeView_GetItemAtPoint(HWND hwndTree, POINT pt);

auto Dialog_SaveFileName(HWND owner
	, char const* filter, char const* defaultFilter, char const* defaultFileName)
	-> std::unique_ptr<string>;

HFONT Font_Create(char const* family, int size);
