#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include "main.h"
#include "VarTreeNodeData_fwd.h"
#include "WrapCall/ModcmdCallInfo.h"
#include "module/Singleton.h"
#include "module/utility.h"

class VTNodeVar
	: public VTNodeData
{
	VTNodeData& parent_;
	string const name_;
	PVal* const pval_;

public:
	VTNodeVar(VTNodeData& parent, string const& name, PVal* pval)
		: parent_(parent), name_(name), pval_(pval)
	{
		assert(pval_);
	}

	auto name() const -> string override { return name_; }
	auto pval() const -> PVal* { return pval_; }

	auto vartype() const -> vartype_t override { return pval_->flag; }

	auto parent() const -> optional_ref<VTNodeData> override
	{
		return &parent_;
	}

	void acceptVisitor(Visitor& visitor) const override { visitor.fVar(*this); }
};

class VTNodeSysvar
	: public VTNodeData
{
	friend class VTNodeSysvarList; // To initialize this

	hpiutil::Sysvar::Id const id_;

public:
	VTNodeSysvar(hpiutil::Sysvar::Id id)
		: id_(id)
	{}

	auto id() const -> hpiutil::Sysvar::Id { return id_; }
	auto name() const -> string override { return hpiutil::Sysvar::List[id_].name; }
	
	auto vartype() const -> vartype_t override
	{
		return hpiutil::Sysvar::List[id_].type;
	}
	auto parent() const -> optional_ref<VTNodeData> override;

	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvar(*this); }
};

class VTNodeSysvarList
	: public VTNodeData
{
	friend class VTRoot;
	using sysvar_list_t = std::array<unique_ptr<VTNodeSysvar>, hpiutil::Sysvar::Count>;

	unique_ptr<sysvar_list_t const> sysvar_;
public:
	auto sysvarList() const -> sysvar_list_t const& { return *sysvar_; }

	auto name() const -> string override { return "+sysvar"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fSysvarList(*this); }

protected:
	void init() override;
	bool updateSub(bool deep) override;
};

class VTNodeScript
	: public VTNodeData
{
	friend class VTRoot;
	struct Impl;
	unique_ptr<Impl> p_;

	VTNodeScript();
public:
	~VTNodeScript();

	auto resolveRefName(string const& fileRefName) const -> shared_ptr<string const>;
	auto fetchScriptAll(char const* fileRefName) const -> optional_ref<string const>;
	auto fetchScriptLine(char const* fileRefName, size_t lineIndex) const -> unique_ptr<string const>;

	auto name() const -> string override { return "+script"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fScript(*this); }
};

class VTNodeLog
	: public VTNodeData
{
	friend class VTRoot;
	struct Impl;
	unique_ptr<Impl> p_;

	VTNodeLog();
public:
	virtual ~VTNodeLog();
	auto str() const -> string const&;
	bool save(char const* fileName) const;
	void clear();
	void append(char const* addition);

	auto name() const -> string override { return "+log"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fLog(*this); }

	struct LogObserver
	{
		virtual ~LogObserver() {}
		virtual void afterAppend(char const* additional) = 0;
	};
	void setLogObserver(weak_ptr<LogObserver>);
};

class VTNodeGeneral
	: public VTNodeData
{
	friend class VTRoot;

public:
	auto name() const -> string override { return "+general"; }
	auto parent() const -> optional_ref<VTNodeData> override;
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

	VTNodeModule(VTNodeData& parent, string const& name);
	virtual ~VTNodeModule();

	auto name() const -> string override;
	auto parent() const -> optional_ref<VTNodeData> override;
	bool updateSub(bool deep) override;

	//foreach
	struct Visitor
	{
		std::function<void(VTNodeModule const&)> fModule;
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
};

// グローバル領域のノード
class VTNodeModule::Global
	: public VTNodeModule
{
	friend class VTRoot;

public:
	static string const Name;
	Global(VTRoot& parent);

private:
	void addVar(const char* name);

protected:
	void init() override;
};

#ifdef with_WrapCall

class VTNodeDynamic
	: public VTNodeData
{
	friend class VTRoot;

	vector<shared_ptr<VTNodeInvoke>> children_;
	shared_ptr<VTNodeResult> independedResult_;
public:
	void addInvokeNode(shared_ptr<VTNodeInvoke> node);
	void eraseLastInvokeNode();
	void addResultNodeIndepended(shared_ptr<VTNodeResult> node);

	auto invokeNodes() const -> decltype(children_) const& { return children_; }
	auto lastIndependedResult() const -> decltype(independedResult_) const& { return independedResult_; }

	void onBgnCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
	auto onEndCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vtype) -> shared_ptr<ResultNodeData const>;

	auto name() const -> string override { return "+dynamic"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fDynamic(*this); }

protected:
	bool updateSub(bool deep) override;
};

class VTNodeInvoke
	: public VTNodeData
{
	WrapCall::ModcmdCallInfo::shared_ptr_type callinfo_;
	vector<shared_ptr<ResultNodeData>> results_;

public:
	VTNodeInvoke(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo)
		: callinfo_(callinfo)
	{}

	auto callinfo() const -> WrapCall::ModcmdCallInfo const& { return *callinfo_; }
	void addResultDepended(shared_ptr<ResultNodeData> const& result);

	auto name() const -> string override { return callinfo_->name(); }
	auto parent() const -> optional_ref<VTNodeData> override;

	void acceptVisitor(Visitor& visitor) const override { visitor.fInvoke(*this); }
protected:
	bool updateSub(bool deep) override;
};

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
	std::weak_ptr<VTNodeInvoke> const invokeDepended;

public:
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vt);
	ResultNodeData(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PVal const* pvResult);

	auto dependedNode() const -> shared_ptr<VTNodeInvoke>;

	auto vartype() const -> vartype_t override { return vtype; }
	auto name() const -> string override { return callinfo->name(); }
	auto parent() const -> optional_ref<VTNodeData> override;

	void acceptVisitor(Visitor& visitor) const override { visitor.fResult(*this); }
};

#endif //defined(with_WrapCall)

class VTRoot
	: public VTNodeData
	, public Singleton<VTRoot>
{
	friend class Singleton<VTRoot>;
	VTRoot();

	struct ChildNodes
	{
		VTNodeModule::Global global_;
		VTNodeDynamic        dynamic_;
		VTNodeSysvarList     sysvarList_;
		VTNodeScript         script_;
		VTNodeGeneral        general_;
		VTNodeLog            log_;

	public:
		ChildNodes(VTRoot& root);
	};
	unique_ptr<ChildNodes> p_;

private:
	auto children() -> std::vector<std::reference_wrapper<VTNodeData>> const&;
public:
	static auto global()     -> VTNodeModule::Global& { return instance().p_->global_; }
	static auto dynamic()    -> VTNodeDynamic       & { return instance().p_->dynamic_; }
	static auto sysvarList() -> VTNodeSysvarList    & { return instance().p_->sysvarList_; }
	static auto script()     -> VTNodeScript        & { return instance().p_->script_; }
	static auto general()    -> VTNodeGeneral       & { return instance().p_->general_; }
	static auto log()        -> VTNodeLog           & { return instance().p_->log_; }

	auto parent() const -> optional_ref<VTNodeData> override { return nullptr; }
	void acceptVisitor(Visitor& visitor) const override { visitor.fRoot(*this); }
protected:
	bool updateSub(bool deep) override;
};

#endif
