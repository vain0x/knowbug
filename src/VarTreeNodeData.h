#ifndef IG_VARTREE_NODE_DATA_H
#define IG_VARTREE_NODE_DATA_H

#include "main.h"
#include "VarTreeNodeData_fwd.h"
#include "StaticVarTree.h"
#include "WrapCall/ModcmdCallInfo.h"

class VTNodeVar
	: public VTNodeData
{};

class VTNodeSysvarList
	: public VTNodeData
	, public Singleton<VTNodeSysvarList>
{
	friend class Singleton<VTNodeSysvarList>;
};

class VTNodeSysvar
	: public VTNodeData
{};

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

	void acceptVisitor(Visitor& visitor) const override { visitor.fResult(*this); }
};

#endif //defined(with_WrapCall)

#endif
