// 関数の返値の処理

#ifndef IG_MODULE_HPIMOD_FUNC_RESULT_H
#define IG_MODULE_HPIMOD_FUNC_RESULT_H

#include <functional>

#include "hsp3plugin_custom.h"

namespace hpimod {

namespace Detail
{
	template<typename TResult, int vtype>
	int SetReffuncResult(PDAT** ppResult, TResult const& src)
	{
		static TResult stt_result;

		assert(ppResult);
		stt_result = src;
		*ppResult = reinterpret_cast<PDAT*>(&stt_result);
		return vtype;
	}
}

//------------------------------------------------
// reffunc の返値を設定する
//-----------------------------------------------
static inline int SetReffuncResult(PDAT** ppResult, label_t const& src) {
	return Detail::SetReffuncResult<label_t, HSPVAR_FLAG_LABEL>(ppResult, src);
}
static inline int SetReffuncResult(PDAT** ppResult, double const& src) {
	return Detail::SetReffuncResult<double, HSPVAR_FLAG_DOUBLE>(ppResult, src);
}
static inline int SetReffuncResult(PDAT** ppResult, int const& src) {
	return Detail::SetReffuncResult<int, HSPVAR_FLAG_INT>(ppResult, src);
}

static char stt_result_string[2048];

static int SetReffuncResult( PDAT** ppResult, char const* const& src )
{
	assert(ppResult);
	strcpy_s( stt_result_string, src );
	*ppResult = reinterpret_cast<PDAT*>(stt_result_string);
	return HSPVAR_FLAG_STR;
//	return SetReffuncResultString( ppResult, [&src](char* p, int size) -> void { strcpy_s(p, size, src); } );
}

static int SetReffuncResultString( PDAT** ppResult, std::function<void(char*, size_t)> lambda )
{
	lambda( stt_result_string, sizeof(stt_result_string) );
	*ppResult = reinterpret_cast<PDAT*>(stt_result_string);
	return HSPVAR_FLAG_STR;
}
} // namespace hpimod

#endif
