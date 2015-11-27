
#pragma once

#include <memory>
#include <functional>

class VTRoot;
class VTNodeModule;
class VTNodeVar;
class VTNodeVector;
class VTNodeValue;
class VTNodeSysvarList;
class VTNodeSysvar;
class VTNodeDynamic;
class VTNodeInvoke;
//class VTNodeResult;
class VTNodeScript;
class VTNodeLog;
class VTNodeGeneral;

struct ResultNodeData;
using VTNodeResult = ResultNodeData;

// ツリービューのノードに対応するクラスのインターフェイス
class VTNodeData
{
public:
	VTNodeData();
	virtual ~VTNodeData();

	// visitor
	struct Visitor
	{
		virtual void fRoot      (VTRoot           const&) {}
		virtual void fModule    (VTNodeModule     const&) {}
		virtual void fVar       (VTNodeVar        const&) {}
		virtual void fVector    (VTNodeVector     const&) {}
		virtual void fValue     (VTNodeValue      const&) {}
		virtual void fSysvarList(VTNodeSysvarList const&) {}
		virtual void fSysvar    (VTNodeSysvar     const&) {}
		virtual void fDynamic   (VTNodeDynamic    const&) {}
		virtual void fInvoke    (VTNodeInvoke     const&) {}
		virtual void fResult    (VTNodeResult     const&) {}
		virtual void fScript    (VTNodeScript     const&) {}
		virtual void fLog       (VTNodeLog        const&) {}
		virtual void fGeneral   (VTNodeGeneral    const&) {}
	};
	virtual void acceptVisitor(Visitor& visitor) const = 0;

	virtual auto parent() const -> optional_ref<VTNodeData> = 0;
	virtual auto name() const -> string { return "(anonymous)"; }
	virtual auto vartype() const -> vartype_t { return HSPVAR_FLAG_NONE; }

	void updateAll() { update(std::numeric_limits<int>::max()); }
	bool update(int nest)     { return updateImpl(true, nest); }
	bool updateDown(int nest) { return updateImpl(false, nest); }
protected:
	/**
	Updates this node and descendants over `nest`-generations.
	Returns `false` if this node or one of the ancestors has vanished.
	If `up` is `true`, also updates its parent;
	otherwise, the parent node is uptodate and still exists.
	//*/
	bool updateImpl(bool up, int nest)
	{
		if ( up && parent() && ! parent()->update(0) ) return false;
		if ( uninitialized_ ) { uninitialized_ = false; onInit(); init(); }
		return updateSub(nest);
	}

	virtual void init() {}
	virtual bool updateSub(int nest) { return true; }
	bool uninitialized_;

private:
	void onInit();

public:
	struct Observer : Visitor
	{
		virtual void onInit(VTNodeData&) {}
		virtual void onTerm(VTNodeData&) {}
	};
	static void registerObserver(weak_ptr<Observer> obs);
};
