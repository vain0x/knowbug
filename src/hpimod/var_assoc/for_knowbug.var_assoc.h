// header for knowbug

#ifndef IG_ASSOC_FOR_KNOWBUG_H
#define IG_ASSOC_FOR_KNOWBUG_H

/*
#ifndef HSP_DEBUGGER
# include "hsp3struct.h"
#else
# include "hspsdk/hsp3struct.h"
#endif
//*/

class CAssoc;

struct StAssocMapList
{
	char  key[128 - sizeof(PVal*) - sizeof(StAssocMapList*)];
	PVal* pval;
	StAssocMapList* next;		// NULL => last
};

using GetMapList_t = StAssocMapList*(*)( CAssoc* );	// HspVarProc::user
static char const* const assoc_vartype_name = "assoc_k";

#endif
