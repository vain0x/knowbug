#include <vector>
#include <set>
#include <map>
#include "main.h"
#include "DebugInfo.h"
#include "StaticVarTree.h"

string const StaticVarTree::ModuleName_Global = "@";

struct StaticVarTree::Private {
	StaticVarTree& self;
	string const name_;
	std::set<string> vars_;
	std::map<string, std::unique_ptr<StaticVarTree>> modules_;

public:
	void pushVar(char const* name);
	void insertVar(char const* name);
	StaticVarTree& insertModule(char const* pModname);
};

StaticVarTree::StaticVarTree(string const& name)
	: p_(new Private { *this, name })
{ }

StaticVarTree::~StaticVarTree() {}

string const& StaticVarTree::getName() const {
	return p_->name_;
}

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void StaticVarTree::Private::pushVar(char const* name)
{
	if ( name[0] == '@' ) return;

	char const* const scopeResolution = std::strchr(name, '@');

	// スコープ解決がある => 子ノードのモジュールに属す
	if ( scopeResolution ) {
		auto& child = insertModule(scopeResolution);
		child.p_->insertVar(name);
	} else {
		insertVar(name);
	}
}

void StaticVarTree::Private::insertVar(char const* name)
{
	vars_.insert(name);
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
StaticVarTree& StaticVarTree::Private::insertModule(char const* pModname)
{
	assert(pModname[0] == '@');

	// '@' を探す (後ろ優先、先頭にはマッチしない)
	char const* const pModnameLast = std::strrchr(&pModname[1], '@');

	// スコープ解決がある場合は、そのモジュール・ノードの子ノードから検索する
	if ( pModnameLast ) {
		// 最後のスコープ解決1つを取り除いた部分
		auto const modname2 = string(pModname, pModnameLast);

		auto& child = insertModule(pModnameLast);
		return child.p_->insertModule(modname2.c_str());

	} else {
		string modname = pModname;
		auto it = modules_.find(modname);
		if ( it == modules_.end() ) {
			it = modules_.emplace(modname, std::make_unique<StaticVarTree>(modname)).first;
		}
		return *it->second;
	}
}

//------------------------------------------------
// 浅い横断
//------------------------------------------------
void StaticVarTree::foreach(StaticVarTree::Visitor const& visitor) const {
	for ( auto&& kv : p_->modules_ ) {
		visitor.fModule(*kv.second);
	}
	for ( auto const& it : p_->vars_ ) {
		visitor.fVar(it);
	}
}

//------------------------------------------------
// グローバルノードを構築する
//------------------------------------------------
StaticVarTree const& StaticVarTree::global() {
	static std::unique_ptr<StaticVarTree> stt_tree;
	if ( !stt_tree ) {
		stt_tree.reset(new StaticVarTree(StaticVarTree::ModuleName_Global));
		auto const&& names = g_dbginfo->fetchStaticVarNames();
		for ( auto const& name : names ) {
			stt_tree->p_->pushVar(name.c_str());
		}
	}
	return *stt_tree;
}
