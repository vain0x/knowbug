//! Windows 系のヘッダーファイルを一括で include する。

#pragma once

#ifdef _WINDOWS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif

#include <Windows.h>
#include <Windowsx.h>
#include <CommCtrl.h>
#include <commdlg.h>
#include <ShellApi.h>
#include <tchar.h>

#undef min
#undef max

#endif
