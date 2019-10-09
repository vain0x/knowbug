
#include "pch.h"
#include <array>
#include <vector>
#include "win_gui.h"

//------------------------------------------------
// 簡易ウィンドウ生成
//------------------------------------------------
auto Window_Create
	( OsStringView className, WNDPROC proc
	, OsStringView caption, int windowStyles
	, int sizeX, int sizeY, int posX, int posY
	, HINSTANCE hInst
	) -> HWND
{
	auto wndclass = WNDCLASS{};
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = proc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInst;
	wndclass.hIcon         = nullptr;
	wndclass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName  = nullptr;
	wndclass.lpszClassName = className.data();
	RegisterClass(&wndclass);

	auto const hWnd =
		CreateWindow
			( className.data(), caption.data()
			, (WS_CAPTION | WS_VISIBLE | windowStyles)
			, posX, posY, sizeX, sizeY
			, /* parent = */ nullptr
			, /* hMenu = */ nullptr
			, hInst
			, /* lparam = */ nullptr
			);
	if ( ! hWnd ) {
		MessageBox(nullptr, TEXT("デバッグウィンドウの初期化に失敗しました。"), TEXT("Knowbug"), 0);
		abort();
	}
	return hWnd;
}

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
	checked = ! checked;
	CheckMenuItem(menu, itemId, (checked ? MF_CHECKED : MF_UNCHECKED));
}

//------------------------------------------------
// EditControl のタブ文字幅を変更する
//------------------------------------------------
void Edit_SetTabLength(HWND hEdit, const int tabwidth)
{
	auto const hdc = GetDC(hEdit);
	{
		auto tm = TEXTMETRIC {};
		if ( GetTextMetrics(hdc, &tm) ) {
			auto const tabstops = tm.tmAveCharWidth / 4 * tabwidth * 2;
			SendMessage(hEdit, EM_SETTABSTOPS, 1, (LPARAM)(&tabstops));
		}
	}
	ReleaseDC(hEdit, hdc);
}

//------------------------------------------------
// ツリービューのフォーカスを回避する
//
// @ 対象のノードが選択状態なら、その兄ノードか親ノードを選択する。
//------------------------------------------------
void TreeView_EscapeFocus(HWND hwndTree, HTREEITEM hItem)
{
	if ( TreeView_GetSelection(hwndTree) == hItem ) {
		auto hUpper = TreeView_GetPrevSibling(hwndTree, hItem);
		if ( ! hUpper ) hUpper = TreeView_GetParent(hwndTree, hItem);

		TreeView_SelectItem(hwndTree, hUpper);
	}
}

//------------------------------------------------
// スクリーン座標 pt にある要素
//------------------------------------------------
auto TreeView_GetItemAtPoint(HWND hwndTree, POINT pt) -> HTREEITEM
{
	auto tvHitTestInfo = TV_HITTESTINFO {};
	tvHitTestInfo.pt = pt;
	ScreenToClient(hwndTree, &tvHitTestInfo.pt);
	auto const hItem = TreeView_HitTest(hwndTree, &tvHitTestInfo);
	return ((tvHitTestInfo.flags & TVHT_ONITEM) != 0)
		? hItem : nullptr;
}

auto Dialog_SaveFileName(
	HWND owner, LPCTSTR filter, LPCTSTR defaultFilter, LPCTSTR defaultFileName
)->std::unique_ptr<OsString>
{
	auto fileName = std::array<TCHAR, MAX_PATH>{};
	auto fullName = std::array<TCHAR, MAX_PATH>{};

	auto ofn = OPENFILENAME {};
	ofn.lStructSize    = sizeof(ofn);
	ofn.hwndOwner      = owner;
	ofn.lpstrFilter    = filter;
	ofn.lpstrFile      = fullName.data();
	ofn.lpstrFileTitle = fileName.data();
	ofn.nMaxFile       = fullName.size();
	ofn.nMaxFileTitle  = fileName.size();
	ofn.Flags          = OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle     = TEXT("名前を付けて保存");
	ofn.lpstrDefExt    = defaultFilter;

	auto ok = GetSaveFileName(&ofn);
	if (!ok) {
		return nullptr;
	}

	return std::make_unique<OsString>(fullName.data());
}

auto Font_Create(OsStringView family, int size, bool antialias) -> HFONT
{
	auto lf = LOGFONT{};
	lf.lfHeight         = -size; // size pt
	lf.lfWidth          = 0;
	lf.lfEscapement     = 0;
	lf.lfOrientation    = 0;
	lf.lfWeight         = FW_NORMAL;
	lf.lfItalic         = FALSE;
	lf.lfUnderline      = FALSE;
	lf.lfStrikeOut      = FALSE;
	lf.lfCharSet        = DEFAULT_CHARSET;
	lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
	lf.lfQuality        = (antialias ? ANTIALIASED_QUALITY : DEFAULT_QUALITY);
	lf.lfPitchAndFamily = DEFAULT_PITCH;

	std::copy(std::begin(family), std::end(family), lf.lfFaceName);

	return CreateFontIndirect(&lf);
}
