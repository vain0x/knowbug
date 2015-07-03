// ModuleTree

#ifndef IG_CLASS_MODULE_TREE_H
#define IG_CLASS_MODULE_TREE_H

#include <string>
#include <memory>
#include <functional>

// モジュールと静的変数からなる木
class StaticVarTree
{
private:
	using string = std::string;
	struct Private;
	std::unique_ptr<Private> p_;

	StaticVarTree();
	StaticVarTree(string const& name);
	~StaticVarTree();

public:
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

public:
	static string const ModuleName_Global;
	static StaticVarTree const& global();
};

#endif
