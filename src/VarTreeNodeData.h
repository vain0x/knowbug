#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include "main.h"
#include "VarTreeNodeData_fwd.h"
#include "StaticVarTree.h"
#include "WrapCall/ModcmdCallInfo.h"

class VTNodeVar
	: public VTNodeData
{
	string const name_;
	PVal* const pval_;

public:
	VTNodeVar(string const& name, PVal* pval)
		: name_(name), pval_(pval)
	{
		assert(pval_);
	}

	auto name() const -> string const& { return name_; }
	auto pval() const -> PVal* { return pval_; }
	auto vartype() const -> vartype_t override { return pval_->flag; }

	void acceptVisitor(Visitor& visitor) const override { visitor.fVar(*this); }
};

class VTNodeSysvar
	: public VTNodeData
{
	friend class VTNodeSysvarList; // To initialize this

	hpiutil::Sysvar::Id id_;
public:
	auto id() const -> hpiutil::Sysvar::Id { return id_; }
	auto name() const -> char const* { return hpiutil::Sysvar::List[id_].name; }
	
	auto vartype() const /* override */ -> vartype_t
	{
		return hpiutil::Sysvar::List[id_].type;
	}

	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvar(*this); }
};

class VTNodeSysvarList
	: public VTNodeData
	, public Singleton<VTNodeSysvarList>
{
	friend class Singleton<VTNodeSysvarList>;
	VTNodeSysvarList();

	std::array<VTNodeSysvar, hpiutil::Sysvar::Count> sysvar_;
public:
	auto sysvarList() const -> decltype(sysvar_) const& { return sysvar_; }

	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvarList(*this); }
};

class VTNodeDynamic
	: public VTNodeData
	, public Singleton<VTNodeDynamic>
{
	friend class Singleton<VTNodeDynamic>;

public:
	void acceptVisitor(Visitor& visitor) const override { visitor.fDynamic(*this); }
};

class VTNodeScript
	: public VTNodeData
	, public Singleton<VTNodeScript>
{
	friend class Singleton<VTNodeScript>;

public:
	void acceptVisitor(Visitor& visitor) const override { visitor.fScript(*this); }
};

class VTNodeLog
	: public VTNodeData
	, public Singleton<VTNodeLog>
{
	friend class Singleton<VTNodeLog>;

public:
	void acceptVisitor(Visitor& visitor) const override { visitor.fLog(*this); }
};

class VTNodeGeneral
	: public VTNodeData
	, public Singleton<VTNodeGeneral>
{
	friend class Singleton<VTNodeGeneral>;

public:
	void acceptVisitor(Visitor& visitor) const override { visitor.fGeneral(*this); }
};

#ifdef with_WrapCall

struct ResultNodeData
	: public VTNodeData
{
	WrapCall::ModcmdCallInfo::shared_ptr_type const callinfo;

	// 返値の型
	vartype_t const vtype;

	// 値の文字列化
	string const treeformedString;
	string const lineformedString;

	// これに依存する呼び出し
	WrapCall::ModcmdCallInfo::shared_ptr_type const pCallInfoDepended;

public:
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vt);
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PVal const* pvResult);

	auto vartype() const -> vartype_t override { return vtype; }

	void acceptVisitor(Visitor& visitor) const override { visitor.fResult(*this); }
};

#endif //defined(with_WrapCall)

#endif
