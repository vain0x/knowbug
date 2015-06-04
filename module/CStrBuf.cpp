
#include <cassert>
#include <algorithm>
#include "CStrBuf.h"

static char const* const stc_warning = "(too long)";
static size_t const stc_warningLength = 10;

CStrBuf::CStrBuf()
	: lenLimit_(0xFFFFFFFF)
{
}

void CStrBuf::limit(size_t newLimit) {
	assert(newLimit >= stc_warningLength);
	lenLimit_ = newLimit;
}

void CStrBuf::append(char const* s)
{
	append(s, std::strlen(s));
}

void CStrBuf::append(char const* s, size_t len)
{
	if ( lenLimit_ == 0 ) return;

	assert(len <= std::strlen(s));
	if ( len + stc_warningLength < lenLimit_ ) {
		buf_.append(s);
		lenLimit_ -= len;
	} else {
		assert( lenLimit_ >= stc_warningLength );
		size_t const lenToWrite = lenLimit_ - stc_warningLength;
		buf_.append(s, lenToWrite);
		buf_.append(stc_warning);
		lenLimit_ = 0;
	}
}
