#include <set>
#include "module/utility.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "VarTreeNodeData.h"

string const VTNodeModule::Global::Name = "@";

struct VTNodeModule::Private
{
	VTNodeModule& self;
	VTNodeData* const parent_;
	string const name_;
	std::map<string, shared_ptr<VTNodeVar>> vars_;
	std::map<string, shared_ptr<VTNodeModule>> modules_;

public:
	void insertVar(char const* name);
	shared_ptr<VTNodeModule> insertModule(char const* pModname);
};

VTNodeModule::VTNodeModule(VTNodeData* parent_, string const& name)
	: p_(new Private { *this, parent_, name })
{ }

VTNodeModule::~VTNodeModule() {}

auto VTNodeModule::name() const -> string
{
	return p_->name_;
}

auto VTNodeModule::parent() const -> shared_ptr<VTNodeData>
{
	return (p_->parent_ ? p_->parent_->shared_from_this() : nullptr);
}

auto VTNodeModule::tryFindVarNode(std::string const& name) const -> shared_ptr<VTNodeVar>
{
	auto&& it = p_->vars_.find(name);
	return ( it != p_->vars_.end() ) ? it->second : nullptr;
}

//------------------------------------------------
// グローバルノードを構築する
//------------------------------------------------
VTNodeModule::Global::Global()
	: VTNodeModule(&VTNodeRoot::instance(), Name)
{}

void VTNodeModule::Global::init()
{
	auto const&& names = g_dbginfo->fetchStaticVarNames();
	for ( auto const& name : names ) {
		addVar(name.c_str());
	}
}

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void VTNodeModule::Global::addVar(char const* name)
{
	char const* const scopeResolution = std::strchr(name, '@');
	if ( scopeResolution ) {
		if ( auto child = p_->insertModule(scopeResolution) ) {
			child->p_->insertVar(name);
		}

	} else {
		p_->insertVar(name);
	}
}

void VTNodeModule::Private::insertVar(char const* name)
{
	PVal* const pval = hpiutil::seekSttVar(name);
	assert(pval);

	vars_.emplace(std::string(name)
		, std::make_shared<VTNodeVar>(&self, std::string(name), pval));
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
shared_ptr<VTNodeModule> VTNodeModule::Private::insertModule(char const* pModname)
{
	assert(pModname[0] == '@');

	//Don't add module whose name begins with a certain prefix.
	if ( g_config->prefixHiddenModule != ""
		&& begins_with(pModname + 0, pModname + strlen(pModname), RANGE_ALL(g_config->prefixHiddenModule)) ) {
		return nullptr;
	}

	char const* const pModnameLast = std::strrchr(&pModname[1], '@');

	if ( pModnameLast ) {
		// 末尾のスコープのモジュールを挿入する
		auto child = insertModule(pModnameLast);
		if ( !child ) return nullptr;

		// スコープを1段除いて、子モジュールに挿入する
		auto const modname2 = string(pModname, pModnameLast);
		return child->p_->insertModule(modname2.c_str());
		
	} else {
		string const modname = pModname;
		auto&& node = map_find_or_insert(modules_, modname, [&]() {
			return std::make_shared<VTNodeModule>(&self, modname);
		});
		return node;
	}
}

//------------------------------------------------
// 浅い横断
//------------------------------------------------
void VTNodeModule::foreach(VTNodeModule::Visitor const& visitor) const {
	for ( auto&& kv : p_->modules_ ) {
		visitor.fModule(kv.second);
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
		for ( auto&& kv : p_->modules_ ) {
			kv.second->updateDeep();
		}
		for ( auto&& kv : p_->vars_ ) {
			kv.second->updateDeep();
		}
	}
	return true;
}
