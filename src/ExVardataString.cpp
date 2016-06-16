// 外部Dll用、VardataString 作成機能

#include "module/ptr_cast.h"
#include "module/CStrBuf.h"
#include "module/CStrWriter.h"

#include "main.h"
#include "ExVardataString.h"
#include "ExVswInternal.h"
#include "CVardataString.h"

// キャスト
static auto vswriter(vswriter_t self) -> CVardataStrWriter&
{
	return *reinterpret_cast<CVardataStrWriter*>(self);
}

EXPORT void WINAPI knowbugVsw_catLeaf(vswriter_t _w, char const* name, char const* value)
{
	vswriter(_w).getWriter().catLeaf(name, value);
}
EXPORT void WINAPI knowbugVsw_catLeafExtra(vswriter_t _w, char const* name, char const* state)
{
	vswriter(_w).getWriter().catLeafExtra(name, state);
}
EXPORT void WINAPI knowbugVsw_catAttribute(vswriter_t _w, char const* name, char const* value)
{
	vswriter(_w).getWriter().catAttribute(name, value);
}
EXPORT void WINAPI knowbugVsw_catNodeBegin(vswriter_t _w, char const* name, char const* leftBracket)
{
	vswriter(_w).getWriter().catNodeBegin(name, leftBracket);
}
EXPORT void WINAPI knowbugVsw_catNodeEnd(vswriter_t _w, char const* rightBracket)
{
	vswriter(_w).getWriter().catNodeEnd(rightBracket);
}

EXPORT void WINAPI knowbugVsw_addVar(vswriter_t _w, char const* name, PVal const* pval)
{
	vswriter(_w).addVar(name, pval);
}
EXPORT void WINAPI knowbugVsw_addVarScalar(vswriter_t _w, char const* name, PVal const* pval, APTR aptr)
{
	vswriter(_w).addVarScalar(name, pval, aptr);
}
EXPORT void WINAPI knowbugVsw_addVarArray(vswriter_t _w, char const* name, PVal const* pval)
{
	vswriter(_w).addVarArray(name, pval);
}

EXPORT void WINAPI knowbugVsw_addValue(vswriter_t _w, char const* name, PDAT const* ptr, /*vartype_t*/ unsigned short vtype)
{
	vswriter(_w).addValue(name, vtype, ptr);
}
EXPORT void WINAPI knowbugVsw_addPrmstack(vswriter_t _w, STRUCTDAT const* stdat, void const* prmstack)
{
	vswriter(_w).addPrmstack(stdat, { prmstack, true });
}
EXPORT void WINAPI knowbugVsw_addStPrm(vswriter_t _w, char const* name, STRUCTPRM const* stprm, void const* ptr)
{
	vswriter(_w).addParameter(name, hpiutil::STRUCTPRM_stdat(stprm), stprm, ptr, true);
}
EXPORT void WINAPI knowbugVsw_addSysvar(vswriter_t _w, char const* name)
{
	auto const id = hpiutil::Sysvar::trySeek(name);
	if ( id != hpiutil::Sysvar::Id::MAX ) {
		vswriter(_w).addSysvar(id);

	} else {
		Knowbug::logmesWarning(strf("システム変数「%s」は存在しない。", name).c_str());
	}
}

EXPORT BOOL WINAPI knowbugVsw_isLineformWriter(vswriter_t _w)
{
	return (vswriter(_w).getWriter().isLineformed() ? TRUE : FALSE);
}

static KnowbugVswMethods g_knowbugVswMethods;
EXPORT auto WINAPI knowbug_getVswMethods() -> KnowbugVswMethods const*
{
	if ( ! g_knowbugVswMethods.catLeaf ) {
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
// HSPからの読み書き用
//------------------------------------------------
EXPORT auto WINAPI knowbugVsw_newTreeformedWriter() -> vswriter_t
{
	return new CVardataStrWriter(
		CVardataStrWriter::create<CTreeformedWriter>(std::make_shared<CStrBuf>())
	);
}

EXPORT auto WINAPI knowbugVsw_newLineformedWriter() -> vswriter_t
{
	return new CVardataStrWriter(
		CVardataStrWriter::create<CLineformedWriter>(std::make_shared<CStrBuf>())
	);
}

EXPORT void WINAPI knowbugVsw_deleteWriter(vswriter_t _w)
{
	if ( _w ) { delete &vswriter(_w); }
}

EXPORT auto WINAPI knowbugVsw_dataPtr(vswriter_t _w, int* length) -> char const*
{
	if ( ! _w ) return nullptr;
	auto& s = vswriter(_w).getString();
	if ( length ) *length = s.size();
	return s.c_str();
}

//------------------------------------------------
// 拙作プラグイン拡張型表示の情報
//------------------------------------------------
auto vswInfoForInternal() -> std::vector<VswInfoForInternal> const&
{
	static std::vector<VswInfoForInternal> vswi {
		{ "int", nullptr, knowbugVsw_addValueInt },
#ifdef with_ExtraBasics
		{ "bool", nullptr, knowbugVsw_addValueBool },
		{ "char", nullptr, knowbugVsw_addValueSChar },
		{ "short", nullptr, knowbugVsw_addValueSShort },
		{ "ushort", nullptr, knowbugVsw_addValueUShort },
		{ "uint", nullptr, knowbugVsw_addValueUInt },
		{ "long", nullptr, knowbugVsw_addValueSLong },
		{ "ulong", nullptr, knowbugVsw_addValueULong },
#endif
#ifdef with_Assoc
		{ "assoc_k", nullptr, knowbugVsw_addValueAssoc },
#endif
#ifdef with_Vector
		{ "vector_k", knowbugVsw_addVarVector, knowbugVsw_addValueVector },
#endif
#ifdef with_Array
		{ "array_k", knowbugVsw_addVarArray, knowbugVsw_addValueArray },
#endif
#ifdef with_Modcmd
		{ "modcmd_k", nullptr, knowbugVsw_addValueModcmd },
#endif
#ifdef with_ModPtr
		{ "int", nullptr, knowbugVsw_addValueIntOrModPtr },
#endif
	};
	return vswi;
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

	if ( ! ptr ) {
		knowbugVsw_catLeafExtra(_w, name, "null_assoc");
		return;
	}

	auto const hvp = hpiutil::tryFindHvp(assoc_vartype_name);
	StAssocMapList* const head = (reinterpret_cast<GetMapList_t>(hvp->user))(src);

	// 要素なし
	if ( ! head ) {
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

	if ( ! src ) {
		knowbugVsw_catLeafExtra(_w, name, "null_vector");
		return;
	}

	auto const hvp = hpiutil::tryFindHvp(vector_vartype_name);
	int len;
	PVal** const pvals = (reinterpret_cast<GetVectorList_t>(hvp->user))(src, &len);

	// 要素なし
	if ( ! pvals ) {
		knowbugVsw_catLeafExtra(_w, name, "empty_vector");
		return;
	}

	// 全キーのリスト
	knowbugVsw_catNodeBegin(_w, name, "[");
	{
		knowbugVsw_catAttribute(_w, "length", strf("%d", len).c_str());

		for ( int i = 0; i < len; ++i ) {
			knowbugVsw_addVar(_w, hpiutil::stringifyArrayIndex({ i }).c_str(), pvals[i]);
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

	if ( ! src ) {
		knowbugVsw_catLeafExtra(_w, name, "null_array");
		return;
	}

	auto const hvp = hpiutil::seekHvp(array_vartype_name);
	PVal* const pvInner = (reinterpret_cast<GetArray_t>(hvp->user))(src);

	// 要素なし
	if ( ! pvInner || pvInner->len[1] == 0 ) {
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

void WINAPI knowbugVsw_addValueModcmd(vswriter_t _w, char const* name, void const* ptr)
{
	auto const modcmd = *reinterpret_cast<int const*>(ptr);
	knowbugVsw_catLeaf(_w, name, strf("modcmd(%s)",
		(modcmd == 0xFFFFFFFF) ? "" : hpiutil::STRUCTDAT_name(&hpiutil::finfo()[modcmd])
	).c_str());
}

#endif

//------------------------------------------------
// modptr の拡張表示
//------------------------------------------------
#ifdef with_ModPtr

#include "with_ModPtr.h"

void WINAPI knowbugVsw_addValueIntOrModPtr(vswriter_t _w, char const* name, void const* ptr)
{
	int const& val = *cptr_cast<int*>(ptr);
	if ( ModPtr::isValid(val) ) {
		auto name2 = strf("%s = mp#%d", name, ModPtr::getIdx(val));
		vswriter(_w).addValueStruct(name2.c_str(), ModPtr::getValue(val));
	} else {
		knowbugVsw_addValueInt(_w, name, ptr);
	}
}
#endif

//------------------------------------------------
// スカラー型の拡張表示
//------------------------------------------------
 void WINAPI knowbugVsw_addValueInt(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<int*>(ptr);
	auto s = (knowbugVsw_isLineformWriter(_w))
		? strf("%d", val)
		: strf("%-10d (0x%08X)", val, val);
	vswriter(_w).getWriter().catLeaf(name, s.c_str());
}

#ifdef with_ExtraBasics

static void catLeaf(vswriter_t _w, char const* name, char const* short_str, char const* long_str)
{
	auto& writer = vswriter(_w).getWriter();
	if ( writer.isLineformed() ) {
		writer.catLeaf(name, short_str);
	} else {
		writer.catLeaf(name, long_str);
	}
}

void WINAPI knowbugVsw_addValueBool(vswriter_t _w, char const* name, void const* ptr)
{
	static char const* const bool_name[2] = { "false", "true" };
	auto& str = bool_name[*cptr_cast<bool*>(ptr) ? 1 : 0];
	catLeaf(_w, name, str, str);
}

void WINAPI knowbugVsw_addValueSChar(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<signed char*>(ptr);
	auto str = (val == 0) ? "0 ('\\0')" : strf("%-3d '%c'", static_cast<int>(val));
	catLeaf(_w, name, str.c_str(), str.c_str());
}

void WINAPI knowbugVsw_addValueSShort(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<signed short*>(ptr);
	catLeaf(_w, name
		, strf("%d", val).c_str()
		, strf("%-6d (0x%04X)", val, static_cast<signed short>(val)).c_str());
}

void WINAPI knowbugVsw_addValueUShort(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<unsigned short*>(ptr);
	catLeaf(_w, name
		, strf("%d", val).c_str()
		, strf("%-6d (0x%04X)", val, static_cast<unsigned short>(val)).c_str());
}

void WINAPI knowbugVsw_addValueUInt(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<unsigned short*>(ptr);
	catLeaf(_w, name
		, strf("%d", val).c_str()
		, strf("%-10d (0x%08X)", val, val).c_str());
}

void WINAPI knowbugVsw_addValueSLong(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<signed long long*>(ptr);
	catLeaf(_w, name
		, strf("%d", val).c_str()
		, strf("%d (0x%16X)", val).c_str());
}

void WINAPI knowbugVsw_addValueULong(vswriter_t _w, char const* name, void const* ptr)
{
	auto const& val = *cptr_cast<unsigned long long*>(ptr);
	catLeaf(_w, name
		, strf("%d", val).c_str()
		, strf("%d (0x%16X)", val).c_str());
}

#endif
