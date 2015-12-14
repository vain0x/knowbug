
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "vartree.h"

static vector<weak_ptr<VTNodeData::Observer>> g_observers;

void VTNodeData::registerObserver(weak_ptr<Observer> obs)
{
	g_observers.emplace_back(std::move(obs));
}

VTNodeData::VTNodeData()
	: uninitialized_(true)
{}

void VTNodeData::onInit()
{
	assert(!uninitialized_);
	for ( auto& wp_obs : g_observers ) {
		if ( auto&& obs = wp_obs.lock() ) {
			obs->onInit(*this);
		}
	}
}

VTNodeData::~VTNodeData()
{
	if ( uninitialized_ ) return;
	for ( auto& wp_obs : g_observers ) {
		if ( auto&& obs = wp_obs.lock() ) {
			obs->onTerm(*this);
		}
	}
}

auto VTNodeSysvar::parent() const -> optional_ref<VTNodeData>
{
	return VTRoot::sysvarList().get();
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

auto VTNodeSysvarList::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
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

auto VTNodeLog::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

auto VTNodeGeneral::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

#ifdef with_WrapCall

using WrapCall::ModcmdCallInfo;

auto VTNodeDynamic::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

void VTNodeDynamic::addInvokeNode(shared_ptr<VTNodeInvoke> node)
{
	independedResult_ = nullptr;

	children_.emplace_back(std::move(node));
}

void VTNodeDynamic::addResultNodeIndepended(shared_ptr<VTNodeResult> node)
{
	independedResult_ = std::move(node);
}

void VTNodeDynamic::eraseLastInvokeNode()
{
	children_.pop_back();
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

void VTNodeDynamic::onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	auto&& node = std::make_shared<VTNodeInvoke>(callinfo);
	addInvokeNode(std::move(node));
}

auto VTNodeDynamic::onEndCalling
	( ModcmdCallInfo::shared_ptr_type const& callinfo
	, PDAT const* ptr, vartype_t vtype)
	-> shared_ptr<ResultNodeData const>
{
	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、他のことをする前に、文字列化などの処理を済ませておく必要がある。
	auto&& pResult =
		(usesResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
		? std::make_shared<ResultNodeData>(callinfo, ptr, vtype)
		: nullptr;

	if ( pResult ) {
		if ( auto&& node = pResult->dependedNode() ) {
			node->addResultDepended(pResult);
		} else {
			addResultNodeIndepended(pResult);
		}
	}

	eraseLastInvokeNode();
	return pResult;
}

auto VTNodeInvoke::parent() const -> optional_ref<VTNodeData>
{
	return VTRoot::dynamic().get();
}

void VTNodeInvoke::addResultDepended(shared_ptr<ResultNodeData> const& result)
{
	results_.emplace_back(result);
}

bool VTNodeInvoke::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto& e : results_ ) {
			e->updateDownDeep();
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

static auto tryFindDependedNode(ModcmdCallInfo const* callinfo) -> shared_ptr<VTNodeInvoke>
{
	if ( callinfo ) {
		if ( auto&& ci_depended = callinfo->tryGetDependedCallInfo() ) {
			auto&& inv = VTRoot::dynamic()->invokeNodes();
			if ( ci_depended->idx < inv.size() ) {
				return inv[ci_depended->idx];
			}
		}
	}
	return nullptr;
}

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PVal const* pvResult)
	: ResultNodeData(callinfo, pvResult->pt, pvResult->flag)
{ }

ResultNodeData::ResultNodeData(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vt)
	: callinfo(callinfo)
	, vtype(vt)
	, invokeDepended(tryFindDependedNode(callinfo.get()))
	, treeformedString(stringFromResultData<CTreeformedWriter>(*callinfo, ptr, vt))
	, lineformedString(stringFromResultData<CLineformedWriter>(*callinfo, ptr, vt))
{ }

auto ResultNodeData::dependedNode() const -> shared_ptr<VTNodeInvoke>
{
	return invokeDepended.lock();
}

auto ResultNodeData::parent() const -> optional_ref<VTNodeData>
{
	if ( auto&& node = dependedNode() ) {
		return node.get();
	} else {
		return VTRoot::dynamic().get();
	}
}

#endif //defined(with_WrapCall)

VTRoot::VTRoot()
	: global_    (new VTNodeModule::Global { *this })
	, dynamic_   (new VTNodeDynamic        {})
	, sysvarList_(new VTNodeSysvarList     {})
	, script_    (new VTNodeScript         {})
	, log_       (new VTNodeLog            {})
	, general_   (new VTNodeGeneral        {})
{}

auto VTRoot::children() -> std::vector<std::weak_ptr<VTNodeData>> const&
{
	static std::vector<std::weak_ptr<VTNodeData>> stt_children =
		{ global_
		, dynamic_
		, sysvarList_
		, script_
		, log_
		, general_
		};
	return stt_children;
}

bool VTRoot::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto&& node_w : children() ) {
			if ( auto&& node = node_w.lock() ) {
				node->updateDownDeep();
			}
		}
	}
	return true;
}
