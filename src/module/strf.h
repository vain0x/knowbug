//formatted string

#ifndef IG_MODULE_STRF_H
#define IG_MODULE_STRF_H

#include <string>

#include "../cppformat/format.h"

//forwarder to adapt interface with hsp
template<typename... Args>
static std::string strf(char const* format, Args&&... args) {
	return fmt::sprintf(format, std::forward<Args>(args)...);
}

#endif
