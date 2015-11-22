
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

void VTNodeData::unregisterObserver(shared_ptr<Observer> obs)
{
	for ( auto& e : g_observers ) {
		if ( e == obs ) { e = std::make_shared<Observer>(); }
	}
}

VTNodeData::VTNodeData()
	: state_(State::Uninit)
{}

VTNodeData::~VTNodeData()
{
	assert(state_ != State::Init);
}

void VTNodeData::onInit()
{
	for ( auto& obs : g_observers ) {
		obs->onInit(*this);
	}
}

void VTNodeData::onTerm()
{
	for ( auto& obs : g_observers ) {
		obs->onTerm(*this);
	}
}

void VTNodeData::terminate()
{
	if ( state_ != State::Init ) return;
	state_ = State::Term;
	terminateSub();
	onTerm();
}

bool VTNodeData::update(bool up, bool deep)
{
	if ( up && parent() && !parent()->updateShallow()
		|| state_ == State::Term ) return false;

	if ( state_ == State::Uninit ) {
		state_ = State::Init;
		onInit();
		init();
	}

	assert(state_ == State::Init);
	return updateSub(deep);
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
			if ( auto&& node = node_w.lock() ) { node->updateDownDeep(); }
		}
	}
	return true;
}

void VTNodeRoot::terminateSub()
{
	for ( auto&& node_w : children() ) {
		if ( auto&& node = node_w.lock() ) {
			node->terminate();
		}
	}
}

auto VTNodeSysvar::parent() const -> shared_ptr<VTNodeData>
{
	return VTNodeSysvarList::make_shared();
}

void VTNodeSysvarList::init()
{
	auto&& sysvars = std::make_unique<sysvar_list_t>();
	for ( size_t i = 0; i < hpiutil::Sysvar::Count; ++i ) {
		auto const id = static_cast<hpiutil::Sysvar::Id>(i);
		sysvars->at(i) = std::make_shared<VTNodeSysvar>(id);
	}

	sysvar_ = std::move(sysvars);
}

bool VTNodeSysvarList::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto&& sysvar : sysvarList() ) {
			sysvar->updateDownDeep();
		}
	}
	return true;
}

void VTNodeSysvarList::terminateSub()
{
	auto&& sysvars = std::move(sysvar_);
	for ( auto&& sysvar : *sysvars ) {
		sysvar->terminate();
	}
}

#ifdef with_WrapCall

using WrapCall::ModcmdCallInfo;

void VTNodeDynamic::addInvokeNode(shared_ptr<VTNodeInvoke> node)
{
	addResultNodeIndepended(nullptr);

	children_.emplace_back(std::move(node));
}

void VTNodeDynamic::addResultNodeIndepended(shared_ptr<VTNodeResult> node)
{
	if ( independedResult_ ) {
		independedResult_->terminate();
	}
	independedResult_ = std::move(node);
}

void VTNodeDynamic::eraseLastInvokeNode()
{
	auto child = std::move(children_.back());
	children_.pop_back();
	child->terminate();
}

bool VTNodeDynamic::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto& e : children_ ) {
			e->updateDownDeep();
		}
		if ( independedResult_ ) {
			independedResult_->updateDownDeep();
		}
	}
	return true;
}

void VTNodeDynamic::terminateSub()
{
	auto children = std::move(children_);
	auto result = std::move(independedResult_);

	for ( auto& e : children ) { e->terminate(); }
	if ( result ) { result->terminate(); }
}

auto VTNodeInvoke::parent() const -> shared_ptr<VTNodeData>
{
	return VTNodeDynamic::make_shared();
}

void VTNodeInvoke::addResultDepended(shared_ptr<ResultNodeData> const& result)
{
	results_.emplace_back(result);
}

bool VTNodeInvoke::updateSub(bool deep)
{
	if ( callinfo().idx >= VTNodeDynamic::make_shared()->invokeNodes().size() ) {
		return false;
	}
	if ( deep ) {
		for ( auto& e : results_ ) {
			e->updateDownDeep();
		}
	}
	return true;
}

void VTNodeInvoke::terminateSub()
{
	auto results = std::move(results_);
	for ( auto& e : results ) { e->terminate(); }
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
