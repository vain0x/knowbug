// VardataString writing functions (for uedai's Dlls)

#pragma once

#include <vector>
#include <string>
#include "ExVardataString.h"

extern void WINAPI knowbugVsw_addValueInt(vswriter_t, char const* name, void const* ptr);

#ifdef with_ExtraBasics
extern void WINAPI knowbugVsw_addValueBool(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueSChar(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueSShort(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueUShort(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueUInt(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueSLong(vswriter_t, char const* name, void const* ptr);
extern void WINAPI knowbugVsw_addValueULong(vswriter_t, char const* name, void const* ptr);
#endif
#ifdef with_ModPtr
extern void WINAPI knowbugVsw_addValueIntOrModPtr(vswriter_t, char const* name, void const* ptr);
#endif
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
