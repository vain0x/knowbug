
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"

VTNodeSysvarList::VTNodeSysvarList()
{
	for ( int i = 0; i < hpiutil::Sysvar::Count; ++i ) {
		sysvar_[i].id_ = static_cast<hpiutil::Sysvar::Id>(i);
	}
}

#ifdef with_WrapCall

using WrapCall::ModcmdCallInfo;

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PVal* pvResult)
	: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
{ }

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT* ptr, vartype_t vt)
	: callinfo(callinfo)
	, vtype(vt)
	, pCallInfoDepended(callinfo->tryGetDependedCallInfo())
{
	{
		auto p = std::make_shared<CStrBuf>();
		CVardataStrWriter::create<CTreeformedWriter>(p)
			.addResult(callinfo->stdat, ptr, vt);
		treeformedString = p->getMove();
	}
	{
		auto p = std::make_shared<CStrBuf>();
		CVardataStrWriter::create<CLineformedWriter>(p)
			.addResult(callinfo->stdat, ptr, vt);
		lineformedString = p->getMove();
	}
}

#endif //defined(with_WrapCall)
