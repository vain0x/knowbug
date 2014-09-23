// VarTree

#include "CVarTree.h"

string const CStaticVarTree::ModuleName_Global = "@";

//------------------------------------------------
// 子ノードとして、変数ノードを追加する
//------------------------------------------------
void CStaticVarTree::ModuleNode::pushVar(char const* name)
{
	// 全スコープ解決を求める
	char const* const pModname = std::strchr(name, '@');

	// スコープ解決がある => 子ノードのモジュールに属す
	if ( pModname ) {
		auto& child = insertChildModule(pModname);
		child.insertChildImpl<VarNode>(name);

	} else {
		insertChildImpl<VarNode>(name);
	}
	return;
}

//------------------------------------------------
// 子ノードの、指定した名前のモジュール・ノードを取得する
// なければ挿入する
//------------------------------------------------
CStaticVarTree::ModuleNode& CStaticVarTree::ModuleNode::insertChildModule(char const* pModname)
{
	assert(pModname[0] == '@');

	// '@' を探す (後ろ優先、先頭にはマッチしない)
	char const* const pModnameLast = std::strrchr(&pModname[1], '@');

	// スコープ解決がある場合は、そのモジュール・ノードの子ノードから検索する
	if ( pModnameLast ) {
		// 最後のスコープ解決1つを取り除いた部分
		auto const modname2 = string(pModname, pModnameLast);

		auto& child = insertChildImpl<ModuleNode>(pModnameLast);
		return child.insertChildModule(modname2.c_str());

	} else {
		return insertChildImpl<ModuleNode>(pModname);
	}
}
