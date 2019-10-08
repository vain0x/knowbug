#pragma once

#include <memory>
#include "../knowbug_core/platform.h"

struct WinFreeLibraryFn {
	using pointer = HMODULE;
	void operator()(HMODULE p) {
		FreeLibrary(p);
	}
};

struct WinDestroyMenuFn {
	using pointer = HMENU;
	void operator()(HMENU p) {
		DestroyMenu(p);
	}
};

struct WinDestroyWindowFn {
	using pointer = HWND;
	void operator()(HWND p) {
		DestroyWindow(p);
	}
};

struct WinDeleteObjectFn {
	using pointer = HGDIOBJ;
	void operator()(pointer p) {
		DeleteObject(p);
	}
};

using ModuleHandlePtr = std::unique_ptr<HMODULE, WinFreeLibraryFn>;
using MenuHandlePtr = std::unique_ptr<HMENU, WinDestroyMenuFn>;
using WindowHandlePtr = std::unique_ptr<HWND, WinDestroyWindowFn>;
using GdiObjHandlePtr = std::unique_ptr<HGDIOBJ, WinDeleteObjectFn>;
