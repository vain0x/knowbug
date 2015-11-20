
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

auto VTNodeRoot::children() -> std::vector<std::weak_ptr<VTNodeData>> const&
{
	static std::vector<std::weak_ptr<VTNodeData>> stt_children =
		{ VTNodeModule::Global::make_shared()
#ifdef with_WrapCall
		, VTNodeDynamic::make_shared()
#endif
		, VTNodeSysvarList::make_shared()
		, VTNodeScript::make_shared()
		, VTNodeLog::make_shared()
		, VTNodeGeneral::make_shared()
		};
	return stt_children;
}

bool VTNodeRoot::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto&& node_w : children() ) {
			if ( auto&& node = node_w.lock() ) { node->updateDeep(); }
		}
	}
	return true;
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

void VTNodeDynamic::addInvokeNode(shared_ptr<VTNodeInvoke> node)
{
	independentResult_ = nullptr;

	children_.emplace_back(std::move(node));
}

void VTNodeDynamic::addResultNodeIndepent(shared_ptr<VTNodeResult> node)
{
	independentResult_ = std::move(node);
}

void VTNodeDynamic::eraseLastInvokeNode()
{
	children_.pop_back();
}

bool VTNodeDynamic::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto& e : children_ ) {
			e->updateDeep();
		}
		if ( independentResult_ ) {
			independentResult_->updateDeep();
		}
	}
	return true;
}

auto VTNodeInvoke::parent() const -> shared_ptr<VTNodeData>
{
	return VTNodeDynamic::make_shared();
}

void VTNodeInvoke::addResultDependent(shared_ptr<ResultNodeData> const& result)
{
	results_.emplace_back(result);
}

bool VTNodeInvoke::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto& e : results_ ) {
			e->updateDeep();
		}
	}
	return true;
}

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

auto ResultNodeData::dependedNode() const -> shared_ptr<VTNodeInvoke>
{
	if ( pCallInfoDepended ) {
		auto&& inv = VTNodeDynamic::make_shared()->invokeNodes();
		size_t const idx = pCallInfoDepended->idx;
		if ( idx < inv.size() ) {
			return inv[idx];
		}
	}
	return nullptr;
}

auto ResultNodeData::parent() const -> shared_ptr<VTNodeData>
{
	if ( auto&& node = dependedNode() ) {
		return node;
	} else {
		return VTNodeDynamic::make_shared();
	}
}

#endif //defined(with_WrapCall)
