// knowbug dialogs

#pragma once

#include "main.h"
#include <Windows.h>
#include <CommCtrl.h>

namespace Dialog
{

HWND getVarTreeHandle();

HWND createMain();
void destroyMain();

void update();
bool logsCalling();

namespace LogBox
{
	void add(char const* msg);
}

} // namespace Dialog

// helpers
void Edit_SetTabLength( HWND hEdit, const int tabwidth );
void Edit_UpdateText(HWND hEdit, char const* s);
string TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem );
LPARAM TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem );
void   TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem );
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem);
