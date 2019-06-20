
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

static auto var_name_to_scope_resolution(char const* var_name) -> char const* {
	return std::strchr(var_name, '@');
}

static bool is_hidden_module(char const* scope_resolution) {
	return std::strcmp(scope_resolution, "@__") == 0;
}

string const VTNodeModule::Global::Name { "@" };

struct VTNodeModule::Private
{
	VTNodeModule& self;
	VTNodeData& parent_;
	string const name_;
	HspStaticVars& static_vars_;
	map<string, unique_ptr<VTNodeVar>> vars_;
	map<string, unique_ptr<VTNodeModule>> modules_;

public:
	void insertVar(char const* name);
	auto insertModule(char const* pModname) -> optional_ref<VTNodeModule>;
};

VTNodeModule::VTNodeModule(VTNodeData& parent_, string const& name, HspStaticVars& static_vars)
	: p_(new Private { *this, parent_, name, static_vars })
{}

VTNodeModule::~VTNodeModule()
{}

auto VTNodeModule::name() const -> string
{
	return p_->name_;
}

auto VTNodeModule::parent() const -> optional_ref<VTNodeData>
{
	return &p_->parent_;
}

//------------------------------------------------
// グローバルノードを構築する
//------------------------------------------------
VTNodeModule::Global::Global(VTRoot& parent, HspStaticVars& static_vars)
	: VTNodeModule(parent, Name, static_vars)
{}

void VTNodeModule::Global::init()
{
	auto const& names = p_->static_vars_.get_all_names();
	for ( auto const& name : names ) {
		addVar(name.c_str());
	}
}

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void VTNodeModule::Global::addVar(char const* name)
{
	if ( auto scopeResolution = var_name_to_scope_resolution(name) ) {
		if ( auto child = p_->insertModule(scopeResolution) ) {
			child->p_->insertVar(name);
		}

	} else {
		p_->insertVar(name);
	}
}

void VTNodeModule::Private::insertVar(char const* name)
{
	auto pval = static_vars_.access_by_name(name);
	assert(pval);

	auto static_var_id = static_vars_.find_id(name);
	auto const& module_path = HspObjectPath::get_root(); // FIXME: use path to the module
	auto path =
		static_var_id
			? module_path.new_static_var(*static_var_id)
			: std::shared_ptr<HspObjectPath const>{};

	vars_.emplace(std::string(name)
		, std::make_unique<VTNodeVar>(self, std::string(name), pval, path));
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
auto VTNodeModule::Private::insertModule(char const* pModname)
	-> optional_ref<VTNodeModule>
{
	assert(pModname[0] == '@');

	if (is_hidden_module(pModname)) {
		return nullptr;
	}

	auto module_name = std::string{ pModname };
	auto iter = modules_.find(module_name);
	if (iter == modules_.end()) {
		iter = modules_.emplace_hint(iter, module_name, std::make_unique<VTNodeModule>(self, module_name, static_vars_));
	}
	return iter->second.get();
}

//------------------------------------------------
// 浅い横断
//------------------------------------------------
void VTNodeModule::foreach(VTNodeModule::Visitor const& visitor) const
{
	for ( auto const& kv : p_->modules_ ) {
		visitor.fModule(*kv.second);
	}
	for ( auto const& it : p_->vars_ ) {
		visitor.fVar(it.first);
	}
}

//------------------------------------------------
// 更新
//------------------------------------------------
bool VTNodeModule::updateSub(bool deep)
{
	if ( deep ) {
		for ( auto const& kv : p_->modules_ ) {
			kv.second->updateDownDeep();
		}
		for ( auto const& kv : p_->vars_ ) {
			kv.second->updateDownDeep();
		}
	}
	return true;
}
