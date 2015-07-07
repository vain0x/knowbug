// 関数の返値の処理

#ifndef IG_MODULE_HPIMOD_FUNC_RESULT_H
#define IG_MODULE_HPIMOD_FUNC_RESULT_H

#include <functional>

#include "hsp3plugin_custom.h"
#include "vartype_traits.h"

namespace hpimod {

//------------------------------------------------
// reffunc の返値を設定する
//-----------------------------------------------
template<typename Tag>
static int SetReffuncResult(PDAT** ppResult, VtTraits::const_value_t<Tag>& value, int vtype) {
	assert(ppResult);
	static VtTraits::value_t<Tag> stt_result;
	stt_result = value;
	*ppResult = VtTraits::asPDAT<Tag>(&stt_result);
	return vtype;
};

template<typename T>
static int SetReffuncResult(PDAT** ppResult, T const& value) {
	using Tag = VtTraits::NativeVartypeTag<T>;
	return SetReffuncResult<Tag>(ppResult, value, VtTraits::vartype<Tag>::apply());
}

static char stt_result_string[HSPCTX_REFSTR_MAX];
static int SetReffuncResultString(PDAT** ppResult, std::function<void(char*, size_t)> lambda)
{
	assert(ppResult);
	lambda(stt_result_string, sizeof(stt_result_string));
	*ppResult = VtTraits::asPDAT<vtStr>(stt_result_string);
	return HSPVAR_FLAG_STR;
}
static int SetReffuncResult( PDAT** ppResult, char const* const& src )
{
	return SetReffuncResultString( ppResult,
		[&src](char* p, int size) { strcpy_s(p, size, src); } );
}
} // namespace hpimod

#endif
