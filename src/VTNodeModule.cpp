
#include <map>
#include "encoding.h"
#include "module/utility.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "VarTreeNodeData.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

using std::map;

// NOTE: @__ で始まるモジュールを非表示にする機能は一時的に無効化されている。
// static bool is_hidden_module(char const* scope_resolution) {
// 	return std::strcmp(scope_resolution, "@__") == 0;
// }

VTNodeModule::VTNodeModule(VTNodeData& parent, std::string&& name, std::shared_ptr<HspObjectPath const> path, HspObjects& objects)
	: parent_(parent)
	, objects_(objects)
	, name_(std::move(name))
	, path_(std::move(path))
	, modules_()
	, vars_()
{
}

VTNodeModule::~VTNodeModule()
{}

auto VTNodeModule::name() const -> string
{
	return name_;
}

auto VTNodeModule::parent() const -> optional_ref<VTNodeData>
{
	return &parent_;
}

void VTNodeModule::init() {
	for (auto i = std::size_t{}; i < path_->child_count(objects_); i++) {
		auto&& child_path = path_->child_at(i, objects_);
		auto kind = child_path->kind();

		assert(kind == HspObjectKind::Module || kind == HspObjectKind::StaticVar);

		if (kind == HspObjectKind::Module) {
			auto module_id = child_path->as_module().module_id();
			auto module_name = child_path->name(objects_);
			modules_.emplace_back(*this, std::move(module_name), std::move(child_path), objects_);
			continue;
		}

		{
			assert(kind == HspObjectKind::StaticVar);
			auto static_var_id = child_path->as_static_var().static_var_id();
			auto name = child_path->name(objects_);
			auto pval = objects_.static_var_to_pval(static_var_id);
			vars_.emplace_back(*this, std::move(name), pval, std::move(child_path));
		}
	}
}

// 浅い横断
void VTNodeModule::foreach(VTNodeModule::Visitor const& visitor) const
{
	for ( auto const& node : modules_ ) {
		visitor.fModule(node);
	}
	for ( auto const& node : vars_ ) {
		visitor.fVar(node.name());
	}
}

// 再帰的更新
bool VTNodeModule::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto& node : modules_ ) {
			node.updateDownDeep();
		}
		for ( auto& node : vars_ ) {
			node.updateDownDeep();
		}
	}
	return true;
}

//------------------------------------------------
// グローバルモジュールノード
//------------------------------------------------

VTNodeModule::Global::Global(VTRoot& parent, HspObjects& objects)
	: VTNodeModule(parent, std::string{ "@" }, HspObjectPath::get_root().new_global_module(objects), objects)
{}

void VTNodeModule::Global::init()
{
	// FIXME: override しない
	VTNodeModule::init();
}
