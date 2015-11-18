
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "vartree.h"

auto VTNodeSysvar::parent() const -> shared_ptr<VTNodeData>
{
	return shared_ptr_from_rawptr(&VTNodeSysvarList::instance());
}

VTNodeSysvarList::VTNodeSysvarList()
{
	for ( int i = 0; i < hpiutil::Sysvar::Count; ++i ) {
		sysvar_[i].id_ = static_cast<hpiutil::Sysvar::Id>(i);
	}
}

#ifdef with_WrapCall

using WrapCall::ModcmdCallInfo;

template<typename TWriter>
static string stringFromResultData(ModcmdCallInfo const& callinfo, PDAT const* ptr, vartype_t vt)
{
	auto&& p = std::make_shared<CStrBuf>();
	CVardataStrWriter::create<TWriter>(p)
		.addResult(callinfo.stdat, ptr, vt);
	return p->getMove();
}

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PVal const* pvResult)
	: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
{ }

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vt)
	: callinfo(callinfo)
	, vtype(vt)
	, pCallInfoDepended(callinfo->tryGetDependedCallInfo())
	, treeformedString(stringFromResultData<CTreeformedWriter>(*callinfo, ptr, vt))
	, lineformedString(stringFromResultData<CLineformedWriter>(*callinfo, ptr, vt))
{ }

#endif //defined(with_WrapCall)
