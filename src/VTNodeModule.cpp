
#include <map>
#include "module/utility.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "VarTreeNodeData.h"

using std::map;

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
	if ( auto scopeResolution = std::strchr(name, '@') ) {
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

	//Don't add module whose name begins with a certain prefix.
	if ( g_config->prefixHiddenModule != ""
		&& begins_with(pModname + 0, pModname + strlen(pModname), RANGE_ALL(g_config->prefixHiddenModule)) ) {
		return nullptr;
	}

	if ( auto pModnameLast = std::strrchr(&pModname[1], '@') ) {
		// 末尾のスコープのモジュールを挿入する
		auto child = insertModule(pModnameLast);
		if ( ! child ) return nullptr;

		// スコープを1段除いて、子モジュールに挿入する
		auto modname2 = string(pModname, pModnameLast);
		return child->p_->insertModule(modname2.c_str());
		
	} else {
		auto modname = string { pModname };
		auto& node = map_find_or_insert(modules_, modname, [&]() {
			return std::make_unique<VTNodeModule>(self, modname);
		});
		return node.get();
	}
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
bool VTNodeModule::updateSub(int nest)
{
	if ( nest > 0 ) {
		for ( auto const& kv : p_->modules_ ) {
			kv.second->updateDown(nest - 1);
		}
		for ( auto const& kv : p_->vars_ ) {
			kv.second->updateDown(nest - 1);
		}
	}
	return true;
}
