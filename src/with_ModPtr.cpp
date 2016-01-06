#ifdef with_ModPtr

#include "main.h"
#include "module/ptr_cast.h"
#include "with_ModPtr.h"

namespace ModPtr
{

auto getAllInstanceVar() -> PVal*
{
	static auto stt_pvalAllInstance = static_cast<PVal*>(nullptr);
	if ( ! stt_pvalAllInstance ) {
		stt_pvalAllInstance = hpiutil::seekSttVar(VarName_AllInstance);
		assert(stt_pvalAllInstance != nullptr);
	}
	return stt_pvalAllInstance;
}

auto getValue(int mp) -> FlexValue*
{
	return &ptr_cast<FlexValue*>( getAllInstanceVar()->pt )[ getIdx(mp) ];
}

} // namespace ModPtr

#endif //defined(with_ModPtr)
