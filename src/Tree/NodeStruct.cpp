#pragma once

#include "../main.h"
#include "../module/ptr_cast.h"
#include "Node.h"

namespace DataTree {

static pair_range<stprm_t> STRUCTDAT_params(stdat_t self) 
{
	return make_pair_range
		( hpimod::STRUCTDAT_getStPrm(self)
		, hpimod::STRUCTDAT_getStPrmEnd(self));
}

static pair_range<stprm_t> FlexValue_members(FlexValue const* self)
{
	stdat_t const stdat = hpimod::FlexValue_getModule(self);
	return make_pair_range
		( hpimod::STRUCTDAT_getStPrm(stdat) + 1  // Skip STRUCTTAG parameter
		, hpimod::STRUCTDAT_getStPrmEnd(stdat) );
}

void NodePrmstack::addChildren()
{
	assert(prmstack_);

	for ( auto& param : STRUCTDAT_params(stdat_) ) {
		string const& name = "";
		void const* const member = prmstack_;

		switch ( param.mptype ) {
			case MPTYPE_STRUCTTAG: break;

			//	case MPTYPE_VAR:
			case MPTYPE_SINGLEVAR:
			case MPTYPE_ARRAYVAR: {
				auto const vardata = cptr_cast<MPVarData*>(member);
				if ( param.mptype == MPTYPE_SINGLEVAR ) {
					addChild<NodeValue>(this, name, vardata->pval, vardata->aptr);
				} else {
					addChild<NodeArray>(this, name, vardata->pval);
				}
				break;
			}
			case MPTYPE_LOCALVAR: {
				auto const pval = cptr_cast<PVal*>(member);
				addChild<NodeArray>(this, name, pval);
				break;
			}
			case MPTYPE_MODULEVAR:
			case MPTYPE_IMODULEVAR:
			case MPTYPE_TMODULEVAR: {
				auto const thismod = cptr_cast<MPModVarData*>(member);
				addChild<NodeStruct>(this, name, hpimod::PVal_getPtr(thismod->pval, thismod->aptr));
				break;
			}
			case MPTYPE_LOCALSTRING:
				addChild<NodeValue>(this, name, member, HSPVAR_FLAG_STR);
				break;

			case MPTYPE_DNUM:
				addChild<NodeValue>(this, name, member, HSPVAR_FLAG_DOUBLE);
				break;

			case MPTYPE_INUM:
				addValue(name, member, HSPVAR_FLAG_INT);
				break;

			case MPTYPE_LABEL:
				addValue(name, member, HSPVAR_FLAG_LABEL);
				break;

			default:
				getWriter().catLeaf(name,
					strf("unknown (mptype = %d)", stprm->mptype).c_str()
					);
				break;
		}
	}
	
}

bool NodePrmstack::updateMembers(tree_t childOrNull)
{
	if ( std::distance(RANGE_ALL(members)) != getChildren().size() ) {
		for ( auto& e : members ) {
			addChild<NodeParam>(&e);
		}
	} else if ( !childOrNull ) {
		for ( auto& e : getChildren() ) {
			e->updateStateAll();
		}
	}
}

bool NodeStruct::updateState(tree_t childOrNull)
{
	if ( !getParent()->updateState(this) ) return false;
	if ( auto const& fv = getFv() ) {
		addChild<NodePrmstack>(this);
		return true;

	} else {
		eraseChildrenAll();
		return false;
	}
}

}
