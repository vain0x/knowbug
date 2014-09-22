// header for knowbug

#ifndef IG_REFERENCE_FOR_KNOWBUG_H
#define IG_REFERENCE_FOR_KNOWBUG_H

#ifndef HSP_DEBUGGER
# include "hsp3struct.h"
#else
# include "chruntime/hsp3struct.h"
#endif

class CReference;

struct StReferenceMapList
{
	char  key[128 - sizeof(PVal*) - sizeof(StReferenceMapList*)];
	PVal* pval;
	StReferenceMapList* next;		// NULL => last
	
	typedef StReferenceMapList* (*GetMapList_t)( CReference* );	// HspVarProc::user
};

#endif
