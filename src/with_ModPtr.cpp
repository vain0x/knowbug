// with modptr.as

#include "main.h"
#include "module/mod_cast.h"
#include "with_ModPtr.h"

namespace ModPtr
{

static PVal* stt_pvalAllInstance = nullptr;	// memo
PVal* getAllInstanceVar()
{
	if ( !stt_pvalAllInstance ) {
		int const iVar = exinfo->HspFunc_seekvar(VarName_AllInstance);
		stt_pvalAllInstance = (iVar >= 0)
			? &ctx->mem_var[iVar]
			: nullptr;
	}
	return stt_pvalAllInstance;
}

FlexValue* getValue(int mp)
{
	FlexValue* const fv = &ptr_cast<FlexValue*>( getAllInstanceVar()->pt )[ getIdx(mp) ];
	return fv;
}

/*
EXPORT void WINAPI knowbug_modptr_register(PVal* pval)
{
	stt_pvalAllInstance = pval;
}
//*/

};

