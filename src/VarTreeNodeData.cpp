
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "vartree.h"

static auto g_observers = vector<weak_ptr<VTNodeData::Observer>> {};

void VTNodeData::registerObserver(weak_ptr<Observer> obs)
{
	g_observers.emplace_back(std::move(obs));
}

VTNodeData::VTNodeData()
	: uninitialized_(true)
{}

void VTNodeData::onInit()
{
	assert(! uninitialized_);
	for ( auto& wp_obs : g_observers ) {
		if ( auto obs = wp_obs.lock() ) {
			obs->onInit(*this);
		}
	}
}

VTNodeData::~VTNodeData()
{
	if ( uninitialized_ ) return;
	for ( auto& wp_obs : g_observers ) {
		if ( auto obs = wp_obs.lock() ) {
			obs->onTerm(*this);
		}
	}
}

auto VTNodeSysvar::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::sysvarList();
}

void VTNodeSysvarList::init()
{
	auto sysvars = std::make_unique<sysvar_list_t>();
	for ( auto i = size_t { 0 }; i < hpiutil::Sysvar::Count; ++i ) {
		auto const id = static_cast<hpiutil::Sysvar::Id>(i);
		sysvars->at(i) = std::make_unique<VTNodeSysvar>(id);
	}

	sysvar_ = std::move(sysvars);
}

auto VTNodeSysvarList::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::instance();
}

bool VTNodeSysvarList::updateSub(int nest)
{
	if ( nest > 0 ) {
		for ( auto&& sysvar : sysvarList() ) {
			sysvar->updateDown(nest - 1);
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

void VTNodeDynamic::addResultNodeIndepended(unique_ptr<VTNodeResult> node)
{
	independedResult_ = std::move(node);
}

void VTNodeDynamic::eraseLastInvokeNode()
{
	children_.pop_back();
}

bool VTNodeDynamic::updateSub(int nest)
{
	if ( nest > 0 ) {
		for ( auto& e : children_ ) {
			e->updateDown(nest - 1);
		}
		if ( independedResult_ ) {
			independedResult_->updateDown(nest - 1);
		}
	}
	return true;
}

void VTNodeDynamic::onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	auto node = std::make_shared<VTNodeInvoke>(callinfo);
	addInvokeNode(std::move(node));
}

auto VTNodeDynamic::onEndCalling
	( ModcmdCallInfo::shared_ptr_type const& callinfo
	, PDAT const* ptr, vartype_t vtype
	) -> optional_ref<ResultNodeData const>
{
	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、他のことをする前に、文字列化などの処理を済ませておく必要がある。
	auto resultNode =
		unique_ptr<ResultNodeData>
		{ (usesResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
			? std::make_unique<ResultNodeData>(callinfo, ptr, vtype)
			: nullptr
		};
	auto* const resultRawPtr = resultNode.get();

	if ( resultNode ) {
		if ( auto node = resultNode->dependedNode() ) {
			node->addResultDepended(std::move(resultNode));
		} else {
			addResultNodeIndepended(std::move(resultNode));
		}
	}

	eraseLastInvokeNode();

	// 生存期間は次の呼び出しが起こるまで
	return resultRawPtr;
}

auto VTNodeInvoke::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::dynamic();
}

void VTNodeInvoke::addResultDepended(unique_ptr<ResultNodeData> result)
{
	results_.emplace_back(std::move(result));
}

bool VTNodeInvoke::updateSub(int nest)
{
	if ( nest > 0 ) {
		for ( auto& e : results_ ) {
			e->updateDown(nest - 1);
		}
	}
	return true;
}

template<typename TWriter>
static auto stringFromResultData(ModcmdCallInfo const& callinfo, PDAT const* ptr, vartype_t vt) -> string
{
	auto p = std::make_shared<CStrBuf>();
	CVardataStrWriter::create<TWriter>(p)
		.addResult(callinfo.stdat, ptr, vt);
	return p->getMove();
}

static auto tryFindDependedNode(ModcmdCallInfo const* callinfo) -> shared_ptr<VTNodeInvoke>
{
	if ( callinfo ) {
		if ( auto const& ci_depended = callinfo->tryGetDependedCallInfo() ) {
			auto const& inv = VTRoot::dynamic().invokeNodes();
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
	if ( auto node = dependedNode() ) {
		return node.get();
	} else {
		return &VTRoot::dynamic();
	}
}

#endif //defined(with_WrapCall)

VTRoot::VTRoot()
	: p_ { new ChildNodes {*this} }
{}

VTRoot::ChildNodes::ChildNodes(VTRoot& root)
	: global_ { root }
{}

auto VTRoot::children() -> std::vector<std::reference_wrapper<VTNodeData>> const&
{
	assert(p_);
	static std::vector<std::reference_wrapper<VTNodeData>> stt_children =
		{ std::ref<VTNodeData>(global    ())
		, std::ref<VTNodeData>(dynamic   ())
		, std::ref<VTNodeData>(sysvarList())
		, std::ref<VTNodeData>(script    ())
		, std::ref<VTNodeData>(log       ())
		, std::ref<VTNodeData>(general   ())
		};
	return stt_children;
}

bool VTRoot::updateSub(int nest)
{
	if ( nest > 0 && p_ ) {
		for ( auto const& node : children() ) {
			node.get().updateDown(nest - 1);
		}
	}
	return true;
}
