// VardataString writing functions (for external Dlls)

#ifndef IG_VARDATA_STRING_EXPORT_H
#define IG_VARDATA_STRING_EXPORT_H

#include <Windows.h>
#include "hsp3plugin.h"

struct KnowbugVswMethods;
using vswriter_t = void*;

// functions to get Knowbug APIs
EXPORT auto WINAPI knowbug_getVswMethods() -> KnowbugVswMethods const*;
using knowbug_getVswMethods_t = decltype(knowbug_getVswMethods);

// functions called from knowbug
using receiveVswMethods_t = void(CALLBACK*)(KnowbugVswMethods const*);
using addVarUserdef_t = void(CALLBACK*)(vswriter_t, char const* name, PVal const* pval);
using addValueUserdef_t = void(CALLBACK*)(vswriter_t, char const* name, void const* ptr);

struct KnowbugVswMethods
{
	// writing methods
	void(CALLBACK *catLeaf)(vswriter_t, char const* name, char const* value);
	void(CALLBACK *catLeafExtra)(vswriter_t, char const* name, char const* state);
	void(CALLBACK *catAttribute)(vswriter_t, char const* name, char const* value);
	void(CALLBACK *catNodeBegin)(vswriter_t, char const* name, char const* leftBracket);
	void(CALLBACK *catNodeEnd)(vswriter_t, char const* rightBracket);

	void(CALLBACK *addVar)(vswriter_t, char const* name, PVal const* pval);
	void(CALLBACK *addVarScalar)(vswriter_t, char const* name, PVal const* pval, APTR aptr);
	void(CALLBACK *addVarArray)(vswriter_t, char const* name, PVal const* pval);

	void(CALLBACK *addValue)(vswriter_t, char const* name, PDAT const* ptr, /*vartype_t*/ unsigned short vtype);
	void(CALLBACK *addPrmstack)(vswriter_t, STRUCTDAT const* stdat, void const* prmstack);
	void(CALLBACK *addStPrm)(vswriter_t, char const* name, STRUCTPRM const* stprm, void const* ptr);
	void(CALLBACK *addSysvar)(vswriter_t, char const* name);

	// others
	BOOL (CALLBACK *isLineformWriter)(vswriter_t);
};

#endif
