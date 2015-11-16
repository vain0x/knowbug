#pragma once

#include "Node.h"

namespace DataTree {

NodeModule::NodeModule(tree_t parent, string const& name)
	: Node(parent, name)
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
	addChild<NodeArray>(rawName, hpimod::seekSttVar(fullName));
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
		for ( auto& it : getChildren() ) {
			submod = dynamic_cast<NodeModule*>(it.get());
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
	return addChild<NodeModule>(rawName);
}

NodeGlobal::NodeGlobal(tree_t parent)
	: NodeModule(parent, "")
	, uninitialized_(true)
{}

void NodeGlobal::init()
{
	for ( string const& name : g_dbginfo->fetchStaticVarNames() ) {
		addVar(name.c_str());
	}
}

bool NodeGlobal::updateState(tree_t childOrNull)
{
	if ( uninitialized_ ) { uninitialized_ = false; init(); }
	return this->NodeModule::updateState(childOrNull);
}

string NodeGlobal::unscope(string const& scopedName) const
{
	return (scopedName.back() == '@')
		? scopedName.substr(0, scopedName.length() - 1)
		: scopedName;
}

}
