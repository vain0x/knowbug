// ä÷êîÇÃï‘ílÇÃèàóù

#ifndef IG_MODULE_HPIMOD_FUNC_RESULT_H
#define IG_MODULE_HPIMOD_FUNC_RESULT_H

#include "hsp3plugin_custom.h"

namespace hpimod {

//##########################################################
//        ä÷êîêÈåæ
//##########################################################
template<class TResult>
int SetReffuncResult( void** ppResult, TResult const& src, int vflag )
{
	static TResult stt_result;

	stt_result = src;
	*ppResult = &stt_result;
	return vflag;
}

#define FTM_SetReffuncResult(_tResult, _vartype) \
	static int SetReffuncResult( void** ppResult, _tResult const& src )	\
	{   return SetReffuncResult( ppResult, src, _vartype );   }

FTM_SetReffuncResult( label_t,   HSPVAR_FLAG_LABEL );
FTM_SetReffuncResult( double,    HSPVAR_FLAG_DOUBLE );
FTM_SetReffuncResult( int,       HSPVAR_FLAG_INT );
//FTM_SetReffuncResult( FlexValue, HSPVAR_FLAG_STRUCT );

static char stt_result_string[2048];

template<class TLambda>
static int SetReffuncResultString( void** ppResult, TLambda lambda )
{
	lambda( stt_result_string, sizeof(stt_result_string) );
	*ppResult = stt_result_string;
	return HSPVAR_FLAG_STR;
}

static int SetReffuncResult( void** ppResult, char const* const& src )
{
	strcpy_s( stt_result_string, src );
	*ppResult = stt_result_string;
	return HSPVAR_FLAG_STR;
//	return SetReffuncResultString( ppResult,
//		[&src](char* p, int size) -> void { strcpy_s(p, size, src); } );
}

} // namespace hpimod

#endif
