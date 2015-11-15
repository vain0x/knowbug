
#pragma once

#include <functional>
#include "../main.h"
#include "../module/utility.h"
#include "../module/Singleton.h"
#include "Visitor.h"
#include "Observer.h"

namespace DataTree {

struct TreeObservers
{
	std::function<void(NodeRoot*)> spawnRoot;
	IVisitor* appendObserver;
	IVisitor* removeObserver;
};
extern std::vector<TreeObservers> stt_observers;
extern void registerObserver(TreeObservers obs);

/**
データツリー

HSPの値を木構造として取り扱うインターフェイス
//*/
class Node
{
public:
	Node(tree_t parent, string const& name)
		: parent_(parent), name_(name)
	{}

	virtual ~Node();

	virtual void acceptVisitor(IVisitor& visitor) = 0;

protected:
	template<typename TNode>
	void acceptVisitorTemplate(IVisitor& visitor)
	{
		assert(dynamic_cast<TNode*>(this) != nullptr);
		visitor.visit(static_cast<TNode*>(this));
	}

public:
	/**
	状況に合わせて木構造を更新する

	child != nullptr の場合は、child は必ず直接の子ノードである。
	このときは、子ノードの更新を行わない。child が生存している場合にかぎり true を返す。
	//*/
	virtual bool updateState(tree_t child_opt) = 0;

	void updateStateAll() { updateState(nullptr); }

	// 更新状態
public:
	enum class UpdatedState { None, Shallow, Deep };
	void setUpdatedState(UpdatedState s) { updatedState_ = s; }
	UpdatedState getUpdatedState() const { return updatedState_; }
private:
	UpdatedState updatedState_;

protected:
	using children_t = std::vector<tree_t>;

public:
	tree_t getParent() const { return parent_; }
	string const& getName() const { return name_; }
	children_t const& getChildren() { return children_; }

protected:
	void rename(string const& name) { name_ = name; }
	
	tree_t addChild(tree_t child);
	tree_t replaceChild(children_t::iterator& pos, tree_t child);
	void removeChild(children_t::iterator& pos);
	void removeChildAll();

private:
	tree_t parent_;
	string name_;
	children_t children_;
};

// 仮想的なルートノード
class NodeRoot
	: public Node
	, public Singleton<NodeRoot>
{
	friend class Singleton<NodeRoot>;
	NodeRoot();

	void spawnRoot();

public:
	void acceptVisitor(IVisitor& visitor) override
	{
		visitor.visit(this);
	}

	bool updateState(tree_t chlidOrNull) override;
};

class NodeLoop
	: public Node
{
public:
	NodeLoop(tree_t parent, tree_t ancestor)
		: Node(parent, string("(loop)"))
	{ }

	void acceptVisitor(IVisitor& visitor) override {
		visitor.visit(this);
	}
};

class NodeModule
	: public Node
{
public:
	NodeModule(tree_t parent, string const& name);

	void acceptVisitor(IVisitor& visitor) override
	{
		acceptVisitorTemplate<NodeModule>(visitor);
	}

	bool updateState(tree_t childOrNull) override;

protected:
	void addVar(const char* fullName);
private:
	void addVarUnscoped(char const* fullName, string const& rawName);

	NodeModule& findModule(char const* scopeResolt);
	NodeModule* addModule(string const& rawName);

	virtual bool contains(char const* name) const;
	virtual string unscope(string const& scopedName) const;
};

class NodeGlobal
	: public NodeModule
{
	friend class NodeRoot;
	NodeGlobal(tree_t parent);
	
	bool updateState(tree_t childOrNull) override;

private:
	void init();
	bool contains(char const* name) const override { return true; }
	string unscope(string const& scopedName) const override;

	bool uninitialized_;
};

class NodeArray
	: public Node
{
public:
	NodeArray(tree_t parent, string const& name, PVal* pval);

	void acceptVisitor(IVisitor& visitor) override
	{
		acceptVisitorTemplate<NodeArray>(visitor);
	}
	bool updateState(tree_t childOrNull) override;

	PVal* getPVal() const { return pval_; }

private:
	void addElem(size_t aptr);
	void updateElem(size_t aptr);
	
private:
	PVal* pval_;
	PVal cur_;
};

class NodeValue
	: public Node
{
public:
	NodeValue(tree_t parent, string const& name, PDAT const* pdat, vartype_t vt)
		: Node(parent, name)
		, pdat_(pdat)
		, vt_(vt)
	{}

	void acceptVisitor(IVisitor& visitor) override
	{
		acceptVisitorTemplate<NodeValue>(visitor);
	}

	// Default implementation for scalar types
	bool updateState(tree_t chlidOrNull) override
	{
		return getParent()->updateState(this);
	}

	vartype_t getVartype() const { return vt_; }
	PDAT const* getValptr() const { return pdat_; }
	void setValptr(PDAT const* pdat) { pdat_ = pdat; }

private:
	PDAT const* pdat_;
	vartype_t vt_;
};

/*
class NodePrmStk
	: public IPolyNode
{
public:
	NodePrmStk(tree_t parent, string name, void* prmstk, stdat_t stdat);
	NodePrmStk(tree_t parent, string name, void* prmstk, stprm_t stprm);

public:
	void acceptVisitor(IVisitor& visitor) override {
		acceptVisitorTemplate<NodePrmStk>(visitor);
	}

private:
	void initialize();
	void add( size_t idx, void* member, stprm_t stprm );
	
private:
	void* prmstk_;
	stdat_t stdat_;
	stprm_t stprm_;
};
//*/

}
