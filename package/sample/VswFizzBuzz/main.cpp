
#include <cstdio>
#include <cassert>
#include <string>

// hspsdk へのパスを通しておく必要があることに注意
#include "hsp3plugin.h"
#include "../../../src/ExVardataString.h"

char const* fizzBuzz(int val)
{
	if ( val % 15 == 0 ) {
		return "FizzBuzz";
	} else if ( val % 3 == 0 ) {
		return "Fizz";
	} else if ( val % 5 == 0 ) {
		return "Buzz";
	} else {
		assert(false);
	}
}

static KnowbugVswMethods const* kvswm;
EXPORT void WINAPI receiveVswMethods(KnowbugVswMethods const* vswMethods)
{
	kvswm = vswMethods;
}

EXPORT void WINAPI addValueInt(vswriter_t vsw, char const* name, void const* ptr)
{
	assert(kvswm && ptr);
	int const& val = *static_cast<int const*>(ptr);

	if ( val % 3 == 0 || val % 5 == 0 ) {
		// 「NAME : (STATE)」という形で出力する
		// catLeaf を使わないのは気持ちの問題
		kvswm->catLeafExtra(vsw, name, fizzBuzz(val));

	} else {
		char buf[100];
		std::sprintf(buf, "%d\t(%08X)", val, val);

		// 「NAME = VALUE」という形で出力する
		kvswm->catLeaf(vsw, name, buf);
	}
}
