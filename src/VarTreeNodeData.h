#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include <optional>
#include "encoding.h"
#include "main.h"
#include "VarTreeNodeData_fwd.h"
#include "WrapCall/ModcmdCallInfo.h"
#include "module/Singleton.h"
#include "module/utility.h"
#include "HspObjectPath.h"
#include "HspObjects.h"

class HspObjectPath;
class HspStaticVars;
class LogObserver;
class Logger;
class SourceFileResolver;

class VTNodeVar
	: public VTNodeData
{
	VTNodeData& parent_;
	string const name_;
	PVal* const pval_;

	std::shared_ptr<HspObjectPath const> const path_;

public:
	VTNodeVar(VTNodeData& parent, string const& name, PVal* pval, std::shared_ptr<HspObjectPath const> path)
		: parent_(parent), name_(name), pval_(pval), path_(path)
	{
		assert(pval_);
	}

	auto name() const -> string override { return name_; }
	auto pval() const -> PVal* { return pval_; }

	auto path() const -> std::shared_ptr<HspObjectPath const> const& {
		return path_;
	}

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

	std::shared_ptr<SourceFileResolver> resolver_;
public:
	VTNodeScript(std::shared_ptr<SourceFileResolver> resolver);

	auto resolveRefName(char const* fileRefName) const -> shared_ptr<string const>;
	auto fetchScriptAll(char const* fileRefName) const -> unique_ptr<OsString>;
	auto fetchScriptLine(hpiutil::SourcePos const& spos) const
		-> unique_ptr<string const>;

	auto name() const -> string override { return "+script"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fScript(*this); }
};

class VTNodeLog
	: public VTNodeData
{
	friend class VTRoot;

public:
	VTNodeLog();

	auto content() const->OsStringView;

	auto name() const -> string override { return "+log"; }
	auto parent() const -> optional_ref<VTNodeData> override;
	void acceptVisitor(Visitor& visitor) const override { visitor.fLog(*this); }

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
public:
	class Global;

private:
	VTNodeData& parent_;
	HspObjects& objects_;

	std::shared_ptr<HspObjectPath const> path_;
	std::string name_;
	std::vector<VTNodeModule> modules_;
	std::vector<VTNodeVar> vars_;

public:
	VTNodeModule(VTNodeData& parent, std::shared_ptr<HspObjectPath const> const& path, HspObjects& objects);
	virtual ~VTNodeModule();

	auto name() const -> string override {
		return name_;
	}

	auto parent() const -> optional_ref<VTNodeData> override {
		return &parent_;
	}

	bool updateSub(bool deep) override;

	auto path() const -> std::shared_ptr<HspObjectPath const> const& {
		return path_;
	}

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

protected:
	void init() override;
};

#ifdef with_WrapCall

class VTNodeDynamic
	: public VTNodeData
{
	friend class VTRoot;

	vector<shared_ptr<VTNodeInvoke>> children_;
public:
	void addInvokeNode(shared_ptr<VTNodeInvoke> node);
	void eraseLastInvokeNode();

	auto invokeNodes() const -> decltype(children_) const& { return children_; }

	void onBgnCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
	void onEndCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vtype);

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

public:
	VTNodeInvoke(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo)
		: callinfo_(callinfo)
	{}

	auto callinfo() const -> WrapCall::ModcmdCallInfo const& { return *callinfo_; }

	auto name() const -> string override { return callinfo_->name(); }
	auto parent() const -> optional_ref<VTNodeData> override;

	void acceptVisitor(Visitor& visitor) const override { visitor.fInvoke(*this); }
protected:
	bool updateSub(bool deep) override;
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
		VTNodeModule         global_;
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
	static auto global()     -> VTNodeModule        & { return instance().p_->global_; }
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
