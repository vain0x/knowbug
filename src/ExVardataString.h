// VardataString writing functions (for external Dlls)

#ifndef IG_VARDATA_STRING_EXPORT_H
#define IG_VARDATA_STRING_EXPORT_H

#include <Windows.h>
#include "hsp3plugin.h"

struct KnowbugVswMethods;
typedef void* vswriter_t;

#ifdef HSP_DEBUGGER
// functions for getting Knowbug APIs
EXPORT KnowbugVswMethods const* WINAPI knowbug_getVswMethods();
#endif
using knowbug_getVswMethods_t = KnowbugVswMethods const*(CALLBACK*)();

// functions called from knowbug
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

// internal writers
#ifdef HSP_DEBUGGER

#ifdef with_Assoc
extern void WINAPI knowbugVsw_addValueAssoc(vswriter_t, char const* name, void const* ptr);
#endif
#ifdef with_Vector
extern void WINAPI knowbugVsw_addVarVector(vswriter_t, char const* name, PVal const* pval);
extern void WINAPI knowbugVsw_addValueVector(vswriter_t, char const* name, void const* ptr);
#endif
#ifdef with_Array
extern void WINAPI knowbugVsw_addVarArray(vswriter_t, char const* name, PVal const* pval);
extern void WINAPI knowbugVsw_addValueArray(vswriter_t, char const* name, void const* ptr);
#endif
#ifdef with_Modcmd
extern void WINAPI knowbugVsw_addValueModcmd(vswriter_t, char const* name, void const* ptr);
#endif

#endif

#endif
