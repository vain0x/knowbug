
#pragma once

#include <Windows.h>

namespace Detail {

struct module_deleter
{
	using pointer = HMODULE;
	void operator()(HMODULE p) { FreeLibrary(p); }
};

struct menu_deleter
{
	using pointer = HMENU;
	void operator()(HMENU p) { DestroyMenu(p); }
};

struct window_deleter
{
	using pointer = HWND;
	void operator()(HWND p) { DestroyWindow(p); }
};

} //namespace Detail

using module_handle_t = std::unique_ptr<HMODULE, Detail::module_deleter>;
using menu_handle_t   = std::unique_ptr<HMENU,   Detail::menu_deleter>;
using window_handle_t = std::unique_ptr<HWND,    Detail::window_deleter>;
