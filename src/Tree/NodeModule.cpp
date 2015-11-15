#pragma once

#include "Node.h"

namespace DataTree {

NodeModule::NodeModule(tree_t parent, string const& name)
	: INode(parent, name)
{}

bool NodeModule::updateState(tree_t child_opt)
{
	if ( !child_opt ) {
		for ( auto& child : getChildren() ) {
			child->updateStateAll();
		}
	}
	return true;
}

bool NodeModule::contains(char const* name) const
{
	size_t const lenModule = getName().length();
	size_t const lenName = strlen(name);

	// name の末尾が "@モジュール名" であればよい
	return (lenName > lenModule + 1)
		&& name[lenName - lenModule - 1] == '@'
		&& (&name[lenName - lenModule]) == getName();
}

string NodeModule::unscope(string const& scopedName) const
{
	assert(contains(scopedName.c_str()));
	return scopedName.substr(0, scopedName.length() - (1 + getName().length()));
}

void NodeModule::addVar(char const* name)
{
	char const* const scopeRes = std::strchr(name, '@');
	if ( scopeRes ) {
		auto& mod = findModule(scopeRes);
		mod.addVarUnscoped(name, string(name, scopeRes));

	} else {
		addVarUnscoped(name, string(name));
	}
}

void NodeModule::addVarUnscoped(char const* fullName, string const& rawName)
{
	assert(rawName.find('@') == string::npos);
	addChild(new NodeArray(this, rawName, hpimod::seekSttVar(fullName)));
}

NodeModule& NodeModule::findModule(char const* scopeRes)
{
	assert(scopeRes[0] == '@');
	char const* const submodScopeRes = std::strrchr(scopeRes + 1, '@');
	if ( submodScopeRes ) {
		NodeModule& submod = findModule(submodScopeRes);
		string const subscopeRes(scopeRes, submodScopeRes);
		return submod.findModule(subscopeRes.c_str());
		
	} else {
		string const submodName = &scopeRes[1];

		// モジュール名で検索
		// TODO: unordered_map か何かで探す
		NodeModule* submod = nullptr;
		for ( auto const it : getChildren() ) {
			submod = dynamic_cast<NodeModule*>(it);
			if ( submod && submod->getName() == submodName ) break;
		}
		if ( !submod ) {
			submod = addModule(submodName);
		}
		return *submod;
	}
}

NodeModule* NodeModule::addModule(string const& rawName)
{
	auto const mod = new NodeModule(this, rawName);
	addChild(mod);
	return mod;
}

/**
グローバル領域の初期化
//*/
NodeGlobal::NodeGlobal()
	: NodeModule(nullptr, "@")
{
	spawnRoot();
	for ( string const& name : g_dbginfo->fetchStaticVarNames() ) {
		addVar(name.c_str());
	}
}

string NodeGlobal::unscope(string const& scopedName) const
{
	return (scopedName.back() == '@')
		? scopedName.substr(0, scopedName.length() - 1)
		: scopedName;
}

}
