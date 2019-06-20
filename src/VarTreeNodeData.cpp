
#include "VarTreeNodeData.h"
#include "CVardataString.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"
#include "vartree.h"
#include "SourceFileResolver.h"
#include "HspRuntime.h"

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
	children_.emplace_back(std::move(node));
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
	}
	return true;
}

void VTNodeDynamic::onBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	auto node = std::make_shared<VTNodeInvoke>(callinfo);
	addInvokeNode(std::move(node));
}

void VTNodeDynamic::onEndCalling
	( ModcmdCallInfo::shared_ptr_type const& callinfo
	, PDAT const* ptr, vartype_t vtype
	)
{
	eraseLastInvokeNode();
}

auto VTNodeInvoke::parent() const -> optional_ref<VTNodeData>
{
	return &VTRoot::dynamic();
}

bool VTNodeInvoke::updateSub(bool deep)
{
	if ( deep ) {
	}
	return true;
}

#endif //defined(with_WrapCall)

VTRoot::VTRoot()
	: p_(new ChildNodes{ *this })
{}

VTRoot::ChildNodes::ChildNodes(VTRoot& root)
	: global_(root, HspObjectPath::get_root().new_global_module(Knowbug::get_hsp_runtime().objects()), Knowbug::get_hsp_runtime().objects())
	, log_(Knowbug::get_logger())
	, script_(Knowbug::get_source_file_resolver())
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

bool VTRoot::updateSub(bool deep)
{
	if ( deep && p_ ) {
		for ( auto const& node : children() ) {
			node.get().updateDownDeep();
		}
	}
	return true;
}
