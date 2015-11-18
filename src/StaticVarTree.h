// ModuleTree

#ifndef IG_CLASS_MODULE_TREE_H
#define IG_CLASS_MODULE_TREE_H

#include <string>
#include <memory>
#include <functional>
#include "module/Singleton.h"

// モジュールと静的変数からなる木
class StaticVarTree
{
private:
	using string = std::string;
	struct Private;
	std::unique_ptr<Private> p_;

public:
	class Global;

	StaticVarTree(string const& name);
	virtual ~StaticVarTree();

	string const& getName() const;

	//foreach
	struct Visitor {
		std::function<void(StaticVarTree const&)> fModule;
		std::function<void(string const&)> fVar;
	};
	void foreach(Visitor const&) const;

	template<typename FModule, typename FVar>
	void foreach(FModule&& fModule, FVar&& fVar) const {
		return foreach(Visitor {
			decltype(Visitor::fModule)(fModule),
			decltype(Visitor::fVar)(fVar)
		});
	}
};

// グローバル領域のノード
class StaticVarTree::Global
	: public StaticVarTree
	, public Singleton<StaticVarTree::Global>
{
public:
	static string const Name;
	Global();

private:
	void addVar(const char* name);
};

#endif
