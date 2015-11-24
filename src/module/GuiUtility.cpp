
#include <vector>
#include "GuiUtility.h"

//------------------------------------------------
// ウィンドウを最前面にする
//------------------------------------------------
void Window_SetTopMost(HWND hwnd, bool isTopMost)
{
	SetWindowPos(
		hwnd, (isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST),
		0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE)
		);
}

//------------------------------------------------
// メニュー項目のチェックを反転
//------------------------------------------------
void Menu_ToggleCheck(HMENU menu, UINT itemId, bool& checked)
{
	checked = !checked;
	CheckMenuItem(menu, itemId, (checked ? MF_CHECKED : MF_UNCHECKED));
}

//------------------------------------------------
// EditControl のタブ文字幅を変更する
//------------------------------------------------
void Edit_SetTabLength(HWND hEdit, const int tabwidth)
{
	HDC const hdc = GetDC(hEdit);
	{
		TEXTMETRIC tm;
		if ( GetTextMetrics(hdc, &tm) ) {
			int const tabstops = tm.tmAveCharWidth / 4 * tabwidth * 2;
			SendMessage(hEdit, EM_SETTABSTOPS, 1, (LPARAM)(&tabstops));
		}
	}
	ReleaseDC(hEdit, hdc);
}

//------------------------------------------------
// EditControl の文字列の置き換え
//------------------------------------------------
void Edit_UpdateText(HWND hwnd, char const* s)
{
	int const vscrollBak = Edit_GetFirstVisibleLine(hwnd);
	SetWindowText(hwnd, s);
	Edit_Scroll(hwnd, vscrollBak, 0);
}

void Edit_SetSelLast(HWND hwnd) {
	Edit_SetSel(hwnd, 0, -1);
	Edit_SetSel(hwnd, -1, -1);
}

//------------------------------------------------
// ツリービューの項目ラベルを取得する
//------------------------------------------------
string TreeView_GetItemString(HWND hwndTree, HTREEITEM hItem)
{
	char stmp[256];

	TVITEM ti;
	ti.hItem = hItem;
	ti.mask = TVIF_TEXT;
	ti.pszText = stmp;
	ti.cchTextMax = sizeof(stmp) - 1;

	return (TreeView_GetItem(hwndTree, &ti) ? stmp : "");
}

//------------------------------------------------
// ツリービューのノードに関連する lparam 値を取得する
//------------------------------------------------
LPARAM TreeView_GetItemLParam(HWND hwndTree, HTREEITEM hItem)
{
	TVITEM ti;
	ti.hItem = hItem;
	ti.mask = TVIF_PARAM;

	TreeView_GetItem(hwndTree, &ti);
	return ti.lParam;
}

//------------------------------------------------
// ツリービューのフォーカスを回避する
// 
// @ 対象のノードが選択状態なら、その兄ノードか親ノードを選択する。
//------------------------------------------------
void TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem)
{
	if ( TreeView_GetSelection(hwndTree) == hItem ) {
		HTREEITEM hUpper = TreeView_GetPrevSibling(hwndTree, hItem);
		if ( !hUpper ) hUpper = TreeView_GetParent(hwndTree, hItem);

		TreeView_SelectItem(hwndTree, hUpper);
	}
}

//------------------------------------------------
// 末子ノードを取得する (failure: nullptr)
//------------------------------------------------
HTREEITEM TreeView_GetChildLast(HWND hwndTree, HTREEITEM hItem)
{
	HTREEITEM hLast = TreeView_GetChild(hwndTree, hItem);
	if ( !hLast ) return nullptr;	// error

	for ( HTREEITEM hNext = hLast
		; hNext != nullptr
		; hNext = TreeView_GetNextSibling(hwndTree, hLast)
		) {
		hLast = hNext;
	}
	return hLast;
}
