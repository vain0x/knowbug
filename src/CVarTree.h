// VarTree

// モジュールをノード、静的変数をリーフとする木構造

// このクラスとしては、モジュール名は先頭の '@' を含むとする。
// (そうした方がソートの手間が省ける)
// 変数名はスコープ解決を含むとする。(gvar または lvar@mod)

// 代数的データ型にしようと思ったが、抽象化できないまま放置

#ifndef IG_CLASS_VAR_TREE_H
#define IG_CLASS_VAR_TREE_H

#include "main.h"
#include <string>
#include <list>
#include <vector>
#include <map>
#include <iterator>
#include <memory>

// Remark: don't inherit this class (except for its case classes).
class CStaticVarTree
{
private:
	string name_;
public:
	string const& getName() const { return name_; }

	CStaticVarTree(string const& name) : name_(name) { }
	virtual ~CStaticVarTree() { }

	static string ModuleName_Global;

	// cases
	class VarNode;
	class ModuleNode;

	enum class CaseId { Var, Module };
	virtual CaseId getCaseId() const = 0;
	
	template<typename TCase> struct isCaseClass {
		using TCaseRaw = std::remove_const_t<TCase>;
		static bool const value = std::is_same<TCaseRaw, VarNode>::value || std::is_same<TCaseRaw, ModuleNode>::value;
	};
	template<typename TCase, typename = std::enable_if_t<isCaseClass<TCase>::value>>
	bool isCaseOf() const { return (this->getCaseId() == TCase::GetCaseId()); }

	template<typename TCase, typename = std::enable_if_t<isCaseClass<TCase>::value>>
	TCase const* asCaseOf() const { return (isCaseOf<TCase>() ? reinterpret_cast<TCase const*>(this) : nullptr); }

	template<typename TCase> TCase* asCaseOf() { return const_cast<CStaticVarTree*>(static_cast<CStaticVarTree const*>(this)->asCaseOf<TCase>()); }

	template<typename TResult, typename TFuncVar, typename TFuncModule>
	TResult match(TFuncVar&& fVar, TFuncModule&& fModule) const
	{
		switch ( getCaseId() ) {
			case CaseId::Var: return fVar(*asCaseOf<VarNode>());
			case CaseId::Module: return fModule(*asCaseOf<ModuleNode>());
		}
	}
	// non const 版は省略
	// template<...> TResult match() { 同様 }
};

// Var
class CStaticVarTree::VarNode
	: public CStaticVarTree
{
public:
	VarNode(string const& name)
		: CStaticVarTree(name)
	{ }

	// case
	static CaseId GetCaseId() { return CaseId::Var; }
	CaseId getCaseId() const override { return CaseId::Var; }
};

// Module
class CStaticVarTree::ModuleNode
	: public CStaticVarTree
{
	using map_t = std::map<string, std::unique_ptr<CStaticVarTree>>;
public:
	using iterator = map_t::iterator;
	using const_iterator = map_t::const_iterator;
private:
	std::unique_ptr<map_t> children_;
public:
	ModuleNode(string const& name)
		: CStaticVarTree(name)
		, children_(new map_t)
	{
		assert(name[0] == '@'
			&& std::strchr(name.c_str() + 1, '@') == nullptr);
	}
	/*
	ModuleNode(ModuleNode const& src)
		: CStaticVarTree(src.getName())
		, children_(new map_t(*src.children_))
	{ }
	ModuleNode(ModuleNode&& src)
		: CStaticVarTree(src)
		, children_(std::move(src.children_))
	{ }//*/

	void pushVar(char const* name);
	void pushModule(char const* name) { insertChildModule(name); }

private:
	ModuleNode& insertChildModule(char const* pModname);

	// 子ノードを検索する。なければ追加する。
	template<typename TNode, typename ...TArgs>
	TNode& insertChildImpl(string const& name, TArgs&& ...args)
	{
		auto iter = children_->find(name);
		if ( iter == children_->end() ) {
			iter = children_->insert(
				{ name, std::make_unique<TNode>(name, std::forward<TArgs>(args)...) }
			).first;
		}
		return reinterpret_cast<TNode&>(*iter->second);
	}
	
public:
	// iterators
	const_iterator begin() const { return children_->begin(); }
	const_iterator end() const { return children_->end(); }

	// case
	static CaseId GetCaseId() { return CaseId::Module; }
	CaseId getCaseId() const override { return CaseId::Module; }
};

using CVarTree = CStaticVarTree;

#endif
