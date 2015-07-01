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
	
	typedef StAssocMapList* (*GetMapList_t)( CAssoc* );	// HspVarProc::user
};

#endif
