// ModuleTree

#ifndef IG_CLASS_MODULE_TREE_H
#define IG_CLASS_MODULE_TREE_H

#include <string>
#include <memory>
#include <functional>

class CStaticVarTree {
private:
	using string = std::string;
	struct Private;
	std::unique_ptr<Private> p_;

public:
	CStaticVarTree(string const& name);
	~CStaticVarTree();

	void pushVar(char const* name);

	string const& getName() const;

	struct Visitor {
		std::function<void(CStaticVarTree const&)> fModule;
		std::function<void(string const&)> fVar;
	};
	void foreach(Visitor const&) const;

	//helper
	template<typename FModule, typename FVar>
	void foreach(FModule&& fModule, FVar&& fVar) const { return foreach(Visitor { decltype(Visitor::fModule)(fModule), decltype(Visitor::fVar)(fVar) }); }

public:
	static string const ModuleName_Global;
};

using CVarTree = CStaticVarTree;

#endif
