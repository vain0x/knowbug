#include <set>
#include "module/utility.h"
#include "DebugInfo.h"
#include "config_mng.h"
#include "StaticVarTree.h"
#include "VarTreeNodeData.h"

string const StaticVarTree::Global::Name = "@";

struct StaticVarTree::Private
{
	StaticVarTree& self;
	string const name_;
	std::map<string, shared_ptr<VTNodeVar>> vars_;
	std::map<string, shared_ptr<StaticVarTree>> modules_;

public:
	void insertVar(char const* name);
	shared_ptr<StaticVarTree> insertModule(char const* pModname);
};

StaticVarTree::StaticVarTree(string const& name)
	: p_(new Private { *this, name })
{ }

StaticVarTree::~StaticVarTree() {}

string const& StaticVarTree::getName() const {
	return p_->name_;
}

auto StaticVarTree::tryFindVarNode(std::string const& name) const -> shared_ptr<VTNodeVar>
{
	auto&& it = p_->vars_.find(name);
	return ( it != p_->vars_.end() ) ? it->second : nullptr;
}

//------------------------------------------------
// グローバルノードを構築する
//------------------------------------------------
StaticVarTree::Global::Global()
	: StaticVarTree(Name)
{
	auto const&& names = g_dbginfo->fetchStaticVarNames();
	for ( auto const& name : names ) {
		addVar(name.c_str());
	}
}

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void StaticVarTree::Global::addVar(char const* name)
{
	if ( name[0] == '@' ) return;

	char const* const scopeResolution = std::strchr(name, '@');
	if ( scopeResolution ) {
		if ( auto child = p_->insertModule(scopeResolution) ) {
			child->p_->insertVar(name);
		}

	} else {
		p_->insertVar(name);
	}
}

void StaticVarTree::Private::insertVar(char const* name)
{
	PVal* const pval = hpiutil::seekSttVar(name);
	assert(pval);

	vars_.emplace(std::string(name)
		, std::make_shared<VTNodeVar>(std::string(name), pval));
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
shared_ptr<StaticVarTree> StaticVarTree::Private::insertModule(char const* pModname)
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
		auto&& node = map_find_or_insert(modules_, modname, [&modname]() {
			return std::make_shared<StaticVarTree>(modname);
		});
		return node;
	}
}

//------------------------------------------------
// 浅い横断
//------------------------------------------------
void StaticVarTree::foreach(StaticVarTree::Visitor const& visitor) const {
	for ( auto&& kv : p_->modules_ ) {
		visitor.fModule(kv.second);
	}
	for ( auto const& it : p_->vars_ ) {
		visitor.fVar(it.first);
	}
}
