// ModuleTree

#ifndef IG_CLASS_MODULE_TREE_H
#define IG_CLASS_MODULE_TREE_H

#include <string>
#include <memory>
#include <functional>
#include "VarTreeNodeData_fwd.h"
#include "module/Singleton.h"

// モジュールと静的変数からなる木
class StaticVarTree
	: public VTNodeData
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
	auto tryFindVarNode(string const& name) const -> shared_ptr<VTNodeVar>;

	//foreach
	struct Visitor {
		std::function<void(shared_ptr<StaticVarTree const> const&)> fModule;
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

	// accept
	void acceptVisitor(VTNodeData::Visitor& visitor) const override { visitor.fModule(*this); }
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
