
#pragma once

#include <memory>
#include <functional>

class VTNodeModule;
class VTNodeVar;
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
	: public std::enable_shared_from_this<VTNodeData>
{
public:
	VTNodeData();
	virtual ~VTNodeData();

	// visitor
	struct Visitor
	{
		virtual void fModule    (VTNodeModule     const&) {}
		virtual void fVar       (VTNodeVar        const&) {}
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

	virtual auto parent() const -> shared_ptr<VTNodeData> = 0;
	virtual auto name() const -> string { return "(anonymous)"; }
	virtual auto vartype() const -> vartype_t { return HSPVAR_FLAG_NONE; }

	void updateShallow() { update(false); }
	void updateDeep() { update(true); }

protected:
	/**
	Updates the parent node and this node itself.
	If `deep` is `true`, also updates children.
	Returns `false` if this node vanish.
	//*/
	bool update(bool deep)
	{
		if ( parent() && !parent()->update(false) ) return false;
		if ( uninitialized_ ) { uninitialized_ = false; onInit(); init(); }
		return updateSub(deep);
	}

	virtual void init() {}
	virtual bool updateSub(bool deep) { return true; }
	bool uninitialized_;

private:
	void onInit();

public:
	struct Observer : Visitor
	{
		virtual void onInit(VTNodeData&) {}
		virtual void onTerm(VTNodeData&) {}
	};
	static void registerObserver(shared_ptr<Observer> obs);
};
