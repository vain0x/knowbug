// with modptr.as

#include "main.h"
#include "module/ptr_cast.h"
#include "with_ModPtr.h"

namespace ModPtr
{

PVal* getAllInstanceVar()
{
	static PVal* stt_pvalAllInstance = nullptr;
	if ( !stt_pvalAllInstance ) {
		stt_pvalAllInstance = hpimod::seekSttVar(VarName_AllInstance);
		assert(stt_pvalAllInstance != nullptr);
	}
	return stt_pvalAllInstance;
}

FlexValue* getValue(int mp)
{
	return &ptr_cast<FlexValue*>( getAllInstanceVar()->pt )[ getIdx(mp) ];
}

};
