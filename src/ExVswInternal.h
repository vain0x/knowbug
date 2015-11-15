// VardataString writing functions (for uedai's Dlls)

#pragma once

#include <vector>
#include "ExVardataString.h"

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

struct VswInfoForInternal
{
	string vtname;
	addVarUserdef_t addVar;
	addValueUserdef_t addValue;
};

extern std::vector<VswInfoForInternal> const& vswInfoForInternal();
extern void sendVswMethods(HMODULE hDll);
