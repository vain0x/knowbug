
#include "CTreeUpdater.h"

namespace DataTree
{;

void CTreeUpdater::visit(CLoopNode* t)
{
	if ( isNowDescendant() ) visit0(t->getChild());
}

void CTreeUpdater::visit(CNodeModule* t)
{
	if ( isNowDescendant() ) {
		for ( auto child : t->getChildren() ) {
			visitAsChild(child);
		}
	}
	return;
}

void CTreeUpdater::visit(CNodeVarArray*)
{

}

void CTreeUpdater::visit(CNodeVarElem*)
{
	;
}

void CTreeUpdater::visit(CNodeVarHolder*)
{
	;
}

void CTreeUpdater::visit(CNodeModInst*)
{
	;
}

void CTreeUpdater::visit(CNodeModInstNull*)
{

}

void CTreeUpdater::visit(CNodePrmStk*)
{
	
}

void CTreeUpdater::visit(CNodeLabel*)
{
	
}

void CTreeUpdater::visit(CNodeString*)
{
	
}

void CTreeUpdater::visit(CNodeDouble*)
{
	
}

void CTreeUpdater::visit(CNodeInt*)
{
	
}

void CTreeUpdater::visit(CNodeUnknown*)
{
	
}

}

