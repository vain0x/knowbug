
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"

using WrapCall::ModcmdCallInfo;

ResultNodeData::ResultNodeData(ModcmdCallInfo const& callinfo, PDAT* ptr, vartype_t vt)
	: stdat(callinfo.stdat)
	, vtype(vt)
	, pCallInfoDepended(callinfo.tryGetDependedCallInfo())
{
	auto p = std::make_shared<CStrBuf>();
	CVardataStrWriter::create<CLineformedWriter>(p)
		.addResult(hpimod::STRUCTDAT_getName(stdat), ptr, vt);
	valueString = p->getMove();
}
