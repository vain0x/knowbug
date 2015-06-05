// VarTree

#include <vector>
#include <set>
#include <map>
#include "main.h"
#include "CVarTree.h"

string const CStaticVarTree::ModuleName_Global = "@";

struct CStaticVarTree::Private {
	CStaticVarTree& self;
	string const name_;
	std::set<string> vars_;
	std::map<string, std::unique_ptr<CStaticVarTree>> modules_;

public:
	void insertVar(char const* name);
	CStaticVarTree& insertModule(char const* pModname);
};

CStaticVarTree::CStaticVarTree(string const& name)
	: p_(new Private { *this, name })
{ }

CStaticVarTree::~CStaticVarTree() {}

string const& CStaticVarTree::getName() const {
	return p_->name_;
}

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void CStaticVarTree::pushVar(char const* name)
{
	if ( name[0] == '@' ) return;

	char const* const scopeResolution = std::strchr(name, '@');

	// スコープ解決がある => 子ノードのモジュールに属す
	if ( scopeResolution ) {
		auto& child = p_->insertModule(scopeResolution);
		child.p_->insertVar(name);
	} else {
		p_->insertVar(name);
	}
}

void CStaticVarTree::Private::insertVar(char const* name)
{
	vars_.insert(name);
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
CStaticVarTree& CStaticVarTree::Private::insertModule(char const* pModname)
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
			it = modules_.emplace(modname, std::make_unique<CStaticVarTree>(modname)).first;
		}
		return *it->second;
	}
}

//------------------------------------------------
// 浅い横断
//------------------------------------------------
void CStaticVarTree::foreach(CStaticVarTree::Visitor const& visitor) const {
	for ( auto&& kv : p_->modules_ ) {
		visitor.fModule(*kv.second);
	}
	for ( auto const& it : p_->vars_ ) {
		visitor.fVar(it);
	}
}
