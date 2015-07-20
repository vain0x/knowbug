#ifndef IG_MODULE_HANDLE_HPP
#define IG_MODULE_HANDLE_HPP

#include <Windows.h>

namespace Detail {

struct module_deleter
{
	using pointer = HMODULE;
	void operator()(HMODULE p) { FreeLibrary(p); }
};

} //namespace Detail

using module_handle_t = std::unique_ptr<HMODULE, Detail::module_deleter>;

#endif
