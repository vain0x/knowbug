// knowbug dialogs

#pragma once

#include "main.h"
#include <Windows.h>
#include <CommCtrl.h>

namespace Dialog
{

static char const* const myClass = "KNOWBUG";
static int const TABDLGMAX = 4;

HWND getKnowbugHandle();
HWND getSttCtrlHandle();
HWND getVarTreeHandle();

HWND createMain();
void destroyMain();

void logClear();
void logAdd( char const* msg );
void logAddCrlf();
void logAddCurInf();
bool isLogAutomaticallyUpdated();
bool isLogCallings();
void logSave();
void logSave( char const* filepath );

void update();

void setEditStyle( HWND hEdit );

} // namespace Dialog

// helpers
void Edit_SetTabLength( HWND hEdit, const int tabwidth );
void Edit_UpdateText(HWND hEdit, char const* s);
string TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem );
LPARAM TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem );
void   TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem );
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem);
