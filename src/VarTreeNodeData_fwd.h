
#pragma once

#include <functional>

//class VTNodeModule;
class VTNodeVar;
class VTNodeSysvarList;
class VTNodeSysvar;
class VTNodeDynamic;
//class VTNodeInvoke;
//class VTNodeResult;
class VTNodeScript;
class VTNodeLog;
class VTNodeGeneral;

class StaticVarTree;
using VTNodeModule = StaticVarTree;

namespace WrapCall { struct ModcmdCallInfo; }
using VTNodeInvoke = WrapCall::ModcmdCallInfo;

struct ResultNodeData;
using VTNodeResult = ResultNodeData;

// ツリービューのノードに対応するクラスのインターフェイス
class VTNodeData
{
public:
	~VTNodeData() {}

	// visitor
	struct Visitor
	{
		std::function<void(VTNodeModule     const&)> fModule;
		std::function<void(VTNodeVar        const&)> fVar;
		std::function<void(VTNodeSysvarList const&)> fSysvarList;
		std::function<void(VTNodeSysvar     const&)> fSysvar;
		std::function<void(VTNodeDynamic    const&)> fDynamic;
		std::function<void(VTNodeInvoke     const&)> fInvoke;
		std::function<void(VTNodeResult     const&)> fResult;
		std::function<void(VTNodeScript     const&)> fScript;
		std::function<void(VTNodeLog        const&)> fLog;
		std::function<void(VTNodeGeneral    const&)> fGeneral;
	};
	virtual void acceptVisitor(Visitor& visitor) = 0;
};
