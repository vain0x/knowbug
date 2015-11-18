
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "vartree.h"

static vector<shared_ptr<VTNodeData::Observer>> g_observers;

void VTNodeData::registerObserver(shared_ptr<Observer> obs)
{
	g_observers.emplace_back(std::move(obs));
}

VTNodeData::VTNodeData()
	: uninitialized_(true)
{}

void VTNodeData::onInit()
{
	assert(!uninitialized_);
	for ( auto& obs : g_observers ) {
		obs->onInit(*this);
	}
}

VTNodeData::~VTNodeData()
{
	if ( uninitialized_ ) return;
	for ( auto& obs : g_observers ) {
		obs->onTerm(*this);
	}
}

auto VTNodeSysvar::parent() const -> shared_ptr<VTNodeData>
{
	return VTNodeSysvarList::make_shared();
}

void VTNodeSysvarList::init()
{
	for ( size_t i = 0; i < hpiutil::Sysvar::Count; ++i ) {
		auto const id = static_cast<hpiutil::Sysvar::Id>(i);
		sysvar_.emplace_back(std::make_shared<VTNodeSysvar>(id));
	}
}

bool VTNodeSysvarList::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto&& sysvar : sysvarList() ) {
			sysvar->updateDeep();
		}
	}
	return true;
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
