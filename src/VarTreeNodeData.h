#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include "main.h"
#include "VarTreeNodeData_fwd.h"
#include "WrapCall/ModcmdCallInfo.h"
#include "module/Singleton.h"
#include "module/utility.h"

template<typename T>
struct SharedSingleton
	: public Singleton<T>
{
	static auto make_shared() -> std::shared_ptr<T>
	{
		static auto instance_ = std::make_shared<T>();
		return instance_;
	}
	static T& instance()
	{
		return *make_shared();
	}
};

class VTNodeVar
	: public VTNodeData
{
	VTNodeData* parent_;
	string const name_;
	PVal* const pval_;

public:
	VTNodeVar(VTNodeData* parent, string const& name, PVal* pval)
		: parent_(parent), name_(name), pval_(pval)
	{
		assert(pval_);
	}

	auto name() const -> string override { return name_; }
	auto pval() const -> PVal* { return pval_; }

	auto vartype() const -> vartype_t override { return pval_->flag; }

	auto parent() const -> shared_ptr<VTNodeData> override
	{
		return (parent_ ? parent_->shared_from_this() : nullptr);
	}

	void acceptVisitor(Visitor& visitor) const override { visitor.fVar(*this); }
};

class VTNodeSysvar
	: public VTNodeData
{
	friend class VTNodeSysvarList; // To initialize this

	hpiutil::Sysvar::Id id_;
public:
	auto id() const -> hpiutil::Sysvar::Id { return id_; }
	auto name() const -> string override { return hpiutil::Sysvar::List[id_].name; }
	
	auto vartype() const -> vartype_t override
	{
		return hpiutil::Sysvar::List[id_].type;
	}
	auto parent() const -> shared_ptr<VTNodeData> override;

	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvar(*this); }
};

class VTNodeSysvarList
	: public VTNodeData
	, public SharedSingleton<VTNodeSysvarList>
{
	friend class Singleton<VTNodeSysvarList>;
	VTNodeSysvarList();

	std::array<VTNodeSysvar, hpiutil::Sysvar::Count> sysvar_;
public:
	auto sysvarList() const -> decltype(sysvar_) const& { return sysvar_; }

	auto name() const -> string override { return "+sysvar"; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvarList(*this); }
};

class VTNodeDynamic
	: public VTNodeData
	, public SharedSingleton<VTNodeDynamic>
{
	friend class Singleton<VTNodeDynamic>;

public:
	auto name() const -> string override { return "+dynamic"; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fDynamic(*this); }
};

class VTNodeScript
	: public VTNodeData
	, public SharedSingleton<VTNodeScript>
{
	friend class Singleton<VTNodeScript>;

public:
	auto name() const -> string override { return "+script"; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fScript(*this); }
};

class VTNodeLog
	: public VTNodeData
	, public SharedSingleton<VTNodeLog>
{
	friend class Singleton<VTNodeLog>;

public:
	auto name() const -> string override { return "+log"; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fLog(*this); }
};

class VTNodeGeneral
	: public VTNodeData
	, public SharedSingleton<VTNodeGeneral>
{
	friend class Singleton<VTNodeGeneral>;

public:
	auto name() const -> string override { return "+general"; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fGeneral(*this); }
};

class VTNodeModule
	: public VTNodeData
{
private:
	struct Private;
	std::unique_ptr<Private> p_;

public:
	class Global;

	VTNodeModule(VTNodeData* parent, string const& name);
	virtual ~VTNodeModule();

	auto name() const -> string override;
	auto parent() const -> shared_ptr<VTNodeData> override;

	//foreach
	struct Visitor
	{
		std::function<void(shared_ptr<VTNodeModule const> const&)> fModule;
		std::function<void(string const&)> fVar;
	};
	void foreach(Visitor const&) const;

	template<typename FModule, typename FVar>
	void foreach(FModule&& fModule, FVar&& fVar) const
	{
		return foreach(Visitor {
			decltype(Visitor::fModule)(fModule),
			decltype(Visitor::fVar)(fVar)
		});
	}

	// accept
	void acceptVisitor(VTNodeData::Visitor& visitor) const override { visitor.fModule(*this); }
	shared_ptr<VTNodeVar> tryFindVarNode(string const& name) const;
};

// グローバル領域のノード
class VTNodeModule::Global
	: public VTNodeModule
	, public SharedSingleton<VTNodeModule::Global>
{
public:
	static string const Name;
	Global();

private:
	void addVar(const char* name);
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
	auto name() const -> string override { return callinfo->name(); }

	auto parent() const -> shared_ptr<VTNodeData> override
	{
		return ( pCallInfoDepended )
			? std::static_pointer_cast<VTNodeData>(pCallInfoDepended)
			: VTNodeDynamic::make_shared();
	}

	void acceptVisitor(Visitor& visitor) const override { visitor.fResult(*this); }
};

#endif //defined(with_WrapCall)

#endif
