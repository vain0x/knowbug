
#include <map>
#include "encoding.h"
#include "module/utility.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "VarTreeNodeData.h"

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
	map<string, unique_ptr<VTNodeVar>> vars_;
	map<string, unique_ptr<VTNodeModule>> modules_;

public:
	void insertVar(char const* name);
	auto insertModule(char const* pModname) -> optional_ref<VTNodeModule>;
};

VTNodeModule::VTNodeModule(VTNodeData& parent_, string const& name)
	: p_(new Private { *this, parent_, name })
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
VTNodeModule::Global::Global(VTRoot& parent)
	: VTNodeModule(parent, Name)
{}

void VTNodeModule::Global::init()
{
	auto names = g_dbginfo->fetchStaticVarNames();
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
	auto pval = hpiutil::seekSttVar(name);
	assert(pval);

	vars_.emplace(std::string(name)
		, std::make_unique<VTNodeVar>(self, std::string(name), pval));
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
		iter = modules_.emplace_hint(iter, module_name, std::make_unique<VTNodeModule>(self, module_name));
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
