#ifdef with_WrapCall

#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"

using WrapCall::ModcmdCallInfo;

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PVal* pvResult)
	: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
{ }

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vt)
	: callinfo(callinfo)
	, vtype(vt)
	, pCallInfoDepended(callinfo->tryGetDependedCallInfo())
{
	auto p = std::make_shared<CStrBuf>();
	CVardataStrWriter::create<CLineformedWriter>(p)
		.addResult(hpimod::STRUCTDAT_getName(callinfo->stdat), ptr, vt);
	valueString = p->getMove();
}

#endif //defined(with_WrapCall)
