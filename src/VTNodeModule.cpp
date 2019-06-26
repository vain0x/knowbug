
#include "encoding.h"
#include "VarTreeNodeData.h"
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

// FIXME: @__ で始まるモジュールを非表示にする機能は一時的に無効化されている。

VTNodeModule::VTNodeModule(VTNodeData& parent, std::shared_ptr<HspObjectPath const> const& path, HspObjects& objects)
	: parent_(parent)
	, objects_(objects)
	, path_(std::move(path))
	, name_(path_->name(objects))
	, modules_()
	, vars_()
{
}

VTNodeModule::~VTNodeModule()
{
}

void VTNodeModule::init() {
	for (auto i = std::size_t{}; i < path_->child_count(objects_); i++) {
		auto&& child_path = path_->child_at(i, objects_);
		auto kind = child_path->kind();

		assert(kind == HspObjectKind::Module || kind == HspObjectKind::StaticVar);

		if (kind == HspObjectKind::Module) {
			auto module_id = child_path->as_module().module_id();
			modules_.emplace_back(*this, std::move(child_path), objects_);
			continue;
		}

		{
			assert(kind == HspObjectKind::StaticVar);
			auto name = child_path->name(objects_);
			auto pval = objects_.static_var_path_to_pval(child_path->as_static_var());
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
