// 外部Dll用、VardataString 作成機能

#include "module/CStrWriter.h"

#include "main.h"
#include "ExVardataString.h"
#include "CVardataString.h"

// キャスト
static CVardataStrWriter& vswriter(vswriter_t self) {
	return *reinterpret_cast<CVardataStrWriter*>(self);
}

void knowbugVsw_catLeaf(vswriter_t _w, char const* name, char const* value)
{
	vswriter(_w).getWriter().catLeaf(name, value);
}
void knowbugVsw_catLeafExtra(vswriter_t _w, char const* name, char const* state)
{
	vswriter(_w).getWriter().catLeafExtra(name, state);
}
void knowbugVsw_catAttribute(vswriter_t _w, char const* name, char const* value)
{
	vswriter(_w).getWriter().catAttribute(name, value);
}
void knowbugVsw_catNodeBegin(vswriter_t _w, char const* name, char const* leftBracket)
{
	vswriter(_w).getWriter().catNodeBegin(name, leftBracket);
}
void knowbugVsw_catNodeEnd(vswriter_t _w, char const* rightBracket)
{
	vswriter(_w).getWriter().catNodeEnd(rightBracket);
}

void knowbugVsw_addVar(vswriter_t _w, char const* name, PVal const* pval)
{
	vswriter(_w).addVar(name, pval);
}
void knowbugVsw_addVarScalar(vswriter_t _w, char const* name, PVal const* pval, APTR aptr)
{
	vswriter(_w).addVarScalar(name, pval, aptr);
}
void knowbugVsw_addVarArray(vswriter_t _w, char const* name, PVal const* pval)
{
	vswriter(_w).addVarArray(name, pval);
}

void knowbugVsw_addValue(vswriter_t _w, char const* name, PDAT const* ptr, /*vartype_t*/ unsigned short vtype)
{
	vswriter(_w).addValue(name, vtype, ptr);
}
void knowbugVsw_addPrmstack(vswriter_t _w, STRUCTDAT const* stdat, void const* prmstack)
{
	vswriter(_w).addPrmstack(stdat, prmstack);
}
void knowbugVsw_addStPrm(vswriter_t _w, char const* name, STRUCTPRM const* stprm, void const* ptr)
{
	vswriter(_w).addParameter(name, hpimod::STRUCTPRM_getStDat(stprm), stprm, ptr);
}
void knowbugVsw_addSysvar(vswriter_t _w, char const* name)
{
	auto const id = Sysvar::seek(name);
	if ( id != Sysvar::Id::MAX ) {
		vswriter(_w).addSysvar(id);

	} else {
		Knowbug::logmesWarning(strf("システム変数「%s」は存在しない。", name).c_str());
	}
}

bool knowbugVsw_isLineformWriter(vswriter_t _w)
{
	return vswriter(_w).getWriter().isLineformed();
}

static KnowbugVswMethods g_knowbugVswMethods;
EXPORT KnowbugVswMethods const* WINAPI knowbug_getVswMethods()
{
	if ( !g_knowbugVswMethods.catLeaf ) {
		g_knowbugVswMethods.catLeaf		     = knowbugVsw_catLeaf		;
		g_knowbugVswMethods.catLeafExtra     = knowbugVsw_catLeafExtra	;
		g_knowbugVswMethods.catAttribute     = knowbugVsw_catAttribute  ;
		g_knowbugVswMethods.catNodeBegin     = knowbugVsw_catNodeBegin	;
		g_knowbugVswMethods.catNodeEnd	     = knowbugVsw_catNodeEnd	;
		g_knowbugVswMethods.addVar           = knowbugVsw_addVar		;
		g_knowbugVswMethods.addVarScalar     = knowbugVsw_addVarScalar	;
		g_knowbugVswMethods.addVarArray	     = knowbugVsw_addVarArray	;
		g_knowbugVswMethods.addValue	     = knowbugVsw_addValue		;
		g_knowbugVswMethods.addPrmstack	     = knowbugVsw_addPrmstack	;
		g_knowbugVswMethods.addStPrm	     = knowbugVsw_addStPrm		;
		g_knowbugVswMethods.addSysvar	     = knowbugVsw_addSysvar		;
		g_knowbugVswMethods.isLineformWriter = knowbugVsw_isLineformWriter;
	}
	return &g_knowbugVswMethods;
}

//------------------------------------------------
// 拙作 assoc 型の拡張表示のための関数群
//------------------------------------------------
#ifdef with_Assoc
# include "../../crouton/src/var_assoc/for_knowbug.var_assoc.h"

//void WINAPI knowbugVsw_addVarAssoc(vswriter_t _w, char const* name, PVal const* pval);

void WINAPI knowbugVsw_addValueAssoc(vswriter_t _w, char const* name, void const* ptr)
{
	auto const src = *reinterpret_cast<CAssoc* const*>(ptr);

	if ( !ptr ) {
		knowbugVsw_catLeafExtra(_w, name, "null_assoc");
		return;
	}

	auto const hvp = hpimod::seekHvp(assoc_vartype_name);
	StAssocMapList* const head = (reinterpret_cast<GetMapList_t>(hvp->user))(src);

	// 要素なし
	if ( !head ) {
		knowbugVsw_catLeafExtra(_w, name, "empty_assoc");
		return;
	}

	// 全キーのリスト
	knowbugVsw_catNodeBegin(_w, name, "[");
	{
		// 列挙
		for ( StAssocMapList* list = head; list != nullptr; list = list->next ) {
			if ( knowbugVsw_isLineformWriter(_w) ) {
				// pair: 「key: value...」
				knowbugVsw_catNodeBegin(_w, CStructedStrWriter::stc_strUnused,
					strf("%s: ", list->key).c_str());
				knowbugVsw_addVar(_w, CStructedStrWriter::stc_strUnused, list->pval);
				knowbugVsw_catNodeEnd(_w, "");
			} else {
				knowbugVsw_addVar(_w, list->key, list->pval);
			}
			//	dbgout("%p: key = %s, pval = %p, next = %p", list, list->key, list->pval, list->next );
		}

		// リストの解放
		for ( StAssocMapList* list = head; list != nullptr; ) {
			StAssocMapList* const next = list->next;
			exinfo->HspFunc_free(list);
			list = next;
		}
	}
	knowbugVsw_catNodeEnd(_w, "]");
}
#endif

//------------------------------------------------
// 拙作 vector 型の拡張表示
//------------------------------------------------
#ifdef with_Vector
# include "../../crouton/src/var_vector/for_knowbug.var_vector.h"

void WINAPI knowbugVsw_addVarVector(vswriter_t _w, char const* name, PVal const* pval)
{
	knowbugVsw_addVarScalar(_w, name, pval, 0);
}

void WINAPI knowbugVsw_addValueVector(vswriter_t _w, char const* name, void const* ptr)
{
	auto const src = *reinterpret_cast<CVector* const*>(ptr);

	if ( !src ) {
		knowbugVsw_catLeafExtra(_w, name, "null_vector");
		return;
	}

	auto const hvp = hpimod::seekHvp(vector_vartype_name);
	int len;
	PVal** const pvals = (reinterpret_cast<GetVectorList_t>(hvp->user))(src, &len);

	// 要素なし
	if ( !pvals ) {
		knowbugVsw_catLeafExtra(_w, name, "empty_vector");
		return;
	}

	// 全キーのリスト
	knowbugVsw_catNodeBegin(_w, name, "[");
	{
		knowbugVsw_catAttribute(_w, "length", strf("%d", len).c_str());

		for ( int i = 0; i < len; ++i ) {
			knowbugVsw_addVar(_w, makeArrayIndexString(1, &i).c_str(), pvals[i]);
			//dbgout("%p: idx = %d, pval = %p, next = %p", list, idx, list->pval, list->next );
		}

	}
	knowbugVsw_catNodeEnd(_w, "]");
}
#endif

//------------------------------------------------
// 拙作 array 型の拡張表示
//------------------------------------------------
#ifdef with_Array
# include "crouton/src/var_array/for_knowbug.var_array.h"

void WINAPI knowbugVsw_addVarArray(vswriter_t _w, char const* name, PVal const* pval)
{
	knowbugVsw_addVarScalar(_w, name, pval, 0);
}

void WINAPI knowbugVsw_addValueArray(vswriter_t _w, char const* name, void const* ptr)
{
	auto const src = *reinterpret_cast<CArray**>(ptr);

	if ( !src ) {
		knowbugVsw_catLeafExtra(_w, name, "null_array");
		return;
	}

	auto const hvp = hpimod::seekHvp(array_vartype_name);
	PVal* const pvInner = (reinterpret_cast<GetArray_t>(hvp->user))(src);

	// 要素なし
	if ( !pvInner || pvInner->len[1] == 0 ) {
		knowbugVsw_catLeafExtra(_w, name, "empty_array");
		return;
	}

	// 表示
	knowbugVsw_addVarArray(_w, name, pvInner);
}
#endif

//------------------------------------------------
// 拙作 modcmd 型の拡張表示
//------------------------------------------------
#ifdef with_Modcmd

extern void WINAPI knowbugVsw_addValueModcmd(vswriter_t _w, char const* name, void const* ptr)
{
	int const modcmd = *reinterpret_cast<int const*>(ptr);
	knowbugVsw_catLeaf(_w, name, strf("modcmd(%s)",
		(modcmd == 0xFFFFFFFF) ? "" : hpimod::STRUCTDAT_getName(hpimod::getSTRUCTDAT(modcmd))
	).c_str());
}

#endif
