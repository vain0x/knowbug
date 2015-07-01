// knowbug dialogs

#pragma once

#include "main.h"
#include <Windows.h>
#include <CommCtrl.h>

#include "resource.h"

#define IDU_TAB 100
#define ID_BTN1 1000
#define ID_BTN2 1001
#define ID_BTN3 1002
#define ID_BTN4 1003
#define ID_BTN5 1004

#define DIALOG_X0 5
#define DIALOG_Y0 5
#define DIALOG_X1 366
#define DIALOG_Y1 406
#define DIALOG_Y2 23

#define WND_SX 380
#define WND_SY 480

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

// その他
void Edit_SetTabLength( HWND hEdit, const int tabwidth );
void Edit_EnableWordwrap( HWND hEdit, bool bEnable );
string TreeView_GetItemString( HWND hwndTree, HTREEITEM hItem );
LPARAM TreeView_GetItemLParam( HWND hwndTree, HTREEITEM hItem );
void   TreeView_EscapeFocus( HWND hwndTree, HTREEITEM hItem );
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem);
