
#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <string>
using string = std::string;

void Window_SetTopMost(HWND hwnd, bool isTopMost);

void Edit_SetTabLength(HWND hEdit, const int tabwidth);
void Edit_UpdateText(HWND hEdit, char const* s);

string TreeView_GetItemString(HWND hwndTree, HTREEITEM hItem);
LPARAM TreeView_GetItemLParam(HWND hwndTree, HTREEITEM hItem);
void   TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem);
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem);
