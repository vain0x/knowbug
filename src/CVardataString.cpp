// 変数データのツリー形式文字列

#include <algorithm>
#include "module/ptr_cast.h"
#include "module/CStrWriter.h"
#include "module/CStrBuf.h"
#include "hpimod/vartype_traits.h"
#include "hpimod/stringization.h"

#include "main.h"
#include "CVardataString.h"
#include "CVarinfoText.h"
#include "CAx.h"
#include "SysvarData.h"

#include "with_ModPtr.h"
#include "WrapCall/ResultNodeData.h"

namespace VtTraits { using namespace hpimod::VtTraits; }
using namespace hpimod::VtTraits::InternalVartypeTags;

static string stringizeSimpleValue(vartype_t type, PDAT const* ptr, bool bShort);
static string stringizeExtraScalar(char const* vtname, PDAT const* ptr, bool bShort, bool& caught);
static string nameFromModuleClass(stdat_t stdat, bool bClone);
static string nameFromStPrm(stprm_t stprm, int idx);
static string nameFromLabel(label_t lb);

CVardataStrWriter::CVardataStrWriter(CVardataStrWriter&& src) : writer_(std::move(src.writer_)) {}
CVardataStrWriter::~CVardataStrWriter() {}

string const& CVardataStrWriter::getString() const { return getWriter().get(); }

//------------------------------------------------
// [add][item] 変数
//------------------------------------------------
void CVardataStrWriter::addVar(char const* name, PVal const* pval)
{
	assert(!!pval);
	auto const hvp = hpimod::getHvp(pval->flag);

	if ( pval->flag >= HSPVAR_FLAG_USERDEF ) {
		auto const&& iter = g_config->vswInfo.find(hvp->vartype_name);
		if ( iter != g_config->vswInfo.end() ) {
			if ( addVarUserdef_t const addVar = std::get<1>(iter->second) ) {
				return addVar(this, name, pval);
			}
		}
	}
	
	if ( hpimod::PVal_isStandardArray(pval) ) {
		addVarArray(name, pval);
	} else {
		addVarScalar(name, pval, 0);
	}
}

//------------------------------------------------
// [add][item] 単体変数
// 
// @ 要素の値を出力する。
//------------------------------------------------
void CVardataStrWriter::addVarScalar(char const* name, PVal const* pval)
{
	addValue(name, pval->flag, hpimod::PVal_getPtr(pval));
}

void CVardataStrWriter::addVarScalar(char const* name, PVal const* pval, APTR aptr)
{
	addValue(name, pval->flag, hpimod::PVal_getPtr(pval, aptr));
}

//------------------------------------------------
// [add][item] 標準配列変数
//------------------------------------------------
void CVardataStrWriter::addVarArray(char const* name, PVal const* pval)
{
	auto const hvp = hpimod::getHvp(pval->flag);
	size_t const cntElem = hpimod::PVal_cntElems(pval);

	getWriter().catNodeBegin(name, strf("<%s>[", hvp->vartype_name).c_str());

	// ツリー状文字列の場合
	if ( !getWriter().isLineformed() ) {
		getWriter().catAttribute("type", stringizeVartype(pval).c_str());

		for ( size_t i = 0; i < cntElem; ++i ) {
			auto&& indexes = hpimod::PVal_indexesFromAptr(pval, i);

			// 要素の値を追加
			string const nameChild = hpimod::stringizeArrayIndex(indexes);
			addVarScalar(nameChild.c_str(), pval, i);
		}

	// 一行文字列の場合
	} else if ( cntElem > 0 ) {
		size_t const dim = hpimod::PVal_maxDim(pval);

		// cntElems[1 + i] = 部分i次元配列の要素数
		// (例えば配列 int(2, 3, 4) だと、cntElems = {1, 2, 2*3, 2*3*4, 0})
		size_t cntElems[1 + hpimod::ArrayDimMax] = { 1 };
		for ( size_t i = 0; i < dim && pval->len[i + 1] > 0; ++i ) {
			cntElems[i + 1] = pval->len[i + 1] * cntElems[i];
		}

		addVarArrayRec(pval, cntElems, dim - 1, 0);
	}

	getWriter().catNodeEnd("]");
}

void CVardataStrWriter::addVarArrayRec(PVal const* pval, size_t const (&cntElems)[hpimod::ArrayDimMax + 1], size_t idxDim, APTR aptr_offset)
{
	assert(getWriter().isLineformed());
	for ( int i = 0; i < pval->len[idxDim + 1]; ++i ) {
		// 2次以上 => 配列を出力
		if ( idxDim >= 1 ) {
			getWriter().catNodeBegin(CStructedStrWriter::stc_strUnused, "[");
			addVarArrayRec(pval, cntElems, idxDim - 1, aptr_offset + (i * cntElems[idxDim]));
			getWriter().catNodeEnd("]");

		// 1次 => 各要素を出力
		} else {
			addVarScalar(CStructedStrWriter::stc_strUnused, pval, aptr_offset + i);
		}
	}
}

//------------------------------------------------
// 一般の値
//------------------------------------------------
void CVardataStrWriter::addValue(char const* name, vartype_t type, PDAT const* ptr)
{
	if ( getWriter().inifiniteNesting() ) {
		getWriter().catLeafExtra(name, "too_many_nesting");
		return;
	}

	// 拡張型
	if ( type >= HSPVAR_FLAG_USERDEF ) {
		auto const iter = g_config->vswInfo.find(hpimod::getHvp(type)->vartype_name);
		if ( iter != g_config->vswInfo.end() ) {
			if ( addValueUserdef_t const addValue = std::get<2>(iter->second) ) {
				addValue(this, name, ptr);
				return;
			}
		}
	}

	if ( type == HSPVAR_FLAG_STRUCT ) {
		addValueStruct(name, VtTraits::asValptr<vtStruct>(ptr));

#ifdef with_ModPtr
	} else if ( type == HSPVAR_FLAG_INT && ModPtr::isValid(VtTraits::derefValptr<vtInt>(ptr)) ) {
		auto const modptr = VtTraits::derefValptr<vtInt>(ptr);
		string const name2 = strf("%s = mp#%d", name, ModPtr::getIdx(modptr));
		addValueStruct(name2.c_str(), ModPtr::getValue(modptr));
#endif
	} else {
		auto const&& dbgstr = stringizeSimpleValue(type, ptr, getWriter().isLineformed());
		getWriter().catLeaf(name, dbgstr.c_str());
	}
}

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVardataStrWriter::addValueStruct(char const* name, FlexValue const* fv)
{
	assert(fv);

	if ( !fv->ptr || fv->type == FLEXVAL_TYPE_NONE ) {
		getWriter().catLeafExtra(name, "nullmod");

	} else {
		auto const stdat = hpimod::FlexValue_getModule(fv);
		auto const modclsNameString =
			nameFromModuleClass(stdat, hpimod::FlexValue_isClone(fv));

		getWriter().catNodeBegin(name, (modclsNameString + "{").c_str());
		getWriter().catAttribute("modcls", modclsNameString.c_str());

		addPrmstack(stdat, { fv->ptr, true });

		getWriter().catNodeEnd("}");
	}
}

//------------------------------------------------
// [add][item] prmstack
// 
// @ 中身だけ出力する。
//------------------------------------------------
void CVardataStrWriter::addPrmstack(stdat_t stdat, std::pair<void const*, bool> prmstk)
{
	int prev_mptype = MPTYPE_NONE;
	int i = 0;

	std::for_each(hpimod::STRUCTDAT_getStPrm(stdat), hpimod::STRUCTDAT_getStPrmEnd(stdat), [&](STRUCTPRM const& stprm) {
		auto const member = hpimod::Prmstack_getMemberPtr(prmstk.first, &stprm);

		// if treeformed: put an additional ' ' before 'local' parameters
		if ( !getWriter().isLineformed()
			&& i > 0
			&& (prev_mptype != MPTYPE_LOCALVAR && stprm.mptype == MPTYPE_LOCALVAR)
			) {
			getWriter().cat(" ");
		}

		addParameter(nameFromStPrm(&stprm, i).c_str(), stdat, &stprm, member, prmstk.second);

		// structtag isn't a member
		if ( stprm.mptype != MPTYPE_STRUCTTAG ) { ++i; }
	});
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVardataStrWriter::addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member, bool isSafe)
{
	if ( !isSafe ) {
		switch ( stprm->mptype ) {
			case MPTYPE_STRUCTTAG:
			case MPTYPE_INUM: //safe iff read member as bits array
			case MPTYPE_DNUM:
			case MPTYPE_LABEL:
				break;
			default:
				getWriter().catLeafExtra(name, "unsafe");
				return;
		}
	}

	switch ( stprm->mptype ) {
		case MPTYPE_STRUCTTAG: break;

		//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		case MPTYPE_ARRAYVAR: {
			auto const vardata = cptr_cast<MPVarData*>(member);
			if ( stprm->mptype == MPTYPE_SINGLEVAR ) {
				addVarScalar(name, vardata->pval, vardata->aptr);
			} else {
				addVar(name, vardata->pval);
			}
			break;
		}
		case MPTYPE_LOCALVAR: {
			auto const pval = cptr_cast<PVal*>(member);
			addVar(name, pval);
			break;
		}
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR: {
			auto const thismod = cptr_cast<MPModVarData*>(member);
			auto const fv = VtTraits::asValptr<vtStruct>(hpimod::PVal_getPtr(thismod->pval, thismod->aptr));
			addValueStruct(name, fv);
			break;
		}
		//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
			addValue(name, HSPVAR_FLAG_STR, VtTraits::asPDAT<vtStr>(*cptr_cast<char**>(member)));
			break;

		case MPTYPE_DNUM:
			addValue(name, HSPVAR_FLAG_DOUBLE, VtTraits::asPDAT<vtDouble>(cptr_cast<double*>(member)));
			break;

		case MPTYPE_INUM:
			addValue(name, HSPVAR_FLAG_INT, VtTraits::asPDAT<vtInt>(cptr_cast<int*>(member)));
			break;

		case MPTYPE_LABEL:
			addValue(name, HSPVAR_FLAG_LABEL, VtTraits::asPDAT<vtLabel>(cptr_cast<label_t*>(member)));
			break;

		default:
			getWriter().catLeaf(name,
				strf("unknown (mptype = %d)", stprm->mptype).c_str()
			);
			break;
	}
}

//------------------------------------------------
// [add] システム変数
// 
// @result: メモリダンプするバッファとサイズ
//------------------------------------------------
void CVardataStrWriter::addSysvar(Sysvar::Id id)
{
	char const* const name = Sysvar::List[id].name;

	switch ( id ) {
		case Sysvar::Id::Refstr:
			addValue(name, HSPVAR_FLAG_STR, VtTraits::asPDAT<vtStr>(ctx->refstr));
			break;

		case Sysvar::Id::Refdval:
			addValue(name, HSPVAR_FLAG_DOUBLE, VtTraits::asPDAT<vtDouble>(&ctx->refdval));
			break;

		case Sysvar::Id::Cnt:
			if ( ctx->looplev > 0 ) {
				// int 配列と同じ表示にする
				getWriter().catNodeBegin(name, "<int>[");
				for ( int i = 0; i < ctx->looplev; ++ i ) {
					auto const lvLoop = ctx->looplev - i;
					addValue(strf("#%d", lvLoop).c_str(), HSPVAR_FLAG_INT,
						VtTraits::asPDAT<vtInt>(&ctx->mem_loop[lvLoop].cnt));
				}
				getWriter().catNodeEnd("]");
			} else {
				getWriter().catLeafExtra(name, "out_of_loop");
			}
			break;

		case Sysvar::Id::NoteBuf:
		{
			if ( PVal const* const pval = ctx->note_pval ) {
				APTR const aptr = ctx->note_aptr;

				auto&& indexes = hpimod::PVal_indexesFromAptr(pval, aptr);
				auto&& indexString = hpimod::stringizeArrayIndex(indexes);
				auto const varName = hpimod::nameFromStaticVar(pval);
				string&& name2 = (name
					? strf("%s (%s%s)", name, varName, indexString)
					: strf("%s (%08X%s)", name, address_cast(pval), aptr));
				addVarScalar(name2.c_str(), pval, aptr);
			} else {
				getWriter().catLeafExtra(name, "not_exist");
			}
			break;
		}
		case Sysvar::Id::Thismod:
			if ( auto const fv = Sysvar::tryGetThismod() ) {
				addValueStruct(name, fv);
			} else {
				getWriter().catLeafExtra(name, "nullmod");
			}
			break;
		default:
			if ( Sysvar::List[id].type == HSPVAR_FLAG_INT ) {
				addValue(name, HSPVAR_FLAG_INT, VtTraits::asPDAT<vtInt>(&Sysvar::getIntRef(id)));
				break;
			}
			assert_sentinel;
	};
}

//------------------------------------------------
// [add] 呼び出し
//
// @prm prmstk can be nullptr
//------------------------------------------------
void CVardataStrWriter::addCall(stdat_t stdat, std::pair<void const*, bool> prmstk)
{
	char const* const name = hpimod::STRUCTDAT_getName(stdat);
	addCall(name, stdat, prmstk);
}

void CVardataStrWriter::addCall(char const* name, stdat_t stdat, std::pair<void const*, bool> prmstk)
{
	getWriter().catNodeBegin(name, strf("%s(", name).c_str());
	if ( !prmstk.first ) {
		getWriter().catLeafExtra("arguments", "not_available");
	} else {
		addPrmstack(stdat, prmstk);
	}
	getWriter().catNodeEnd(")");
}

//------------------------------------------------
// [add] 返値
//------------------------------------------------
void CVardataStrWriter::addResult(char const* name, PDAT const* ptr, vartype_t type)
{
	//現在の実装では一行表示でしか呼ばれない
	//ツリー形式にするなら文字列化の方法を考えなおす
	assert(getWriter().isLineformed());

	addValue(name, type, ptr);
}

//------------------------------------------------
// 単純な値を文字列化する
//
// @ addItem_value でフックされる型はここに来ない。
//------------------------------------------------
string stringizeSimpleValue(vartype_t type, PDAT const* ptr, bool bShort)
{
	assert(type != HSPVAR_FLAG_STRUCT);
	assert(ptr);

	switch ( type ) {
		case HSPVAR_FLAG_STR: {
			char const* const s = VtTraits::asValptr<vtStr>(ptr);
			return (bShort
				? hpimod::literalFormString(s)
				: string(s));
		}
		case HSPVAR_FLAG_COMOBJ:  return strf("comobj(0x%08X)", address_cast(*cptr_cast<void**>(ptr)));
		case HSPVAR_FLAG_VARIANT: return strf("variant(0x%08X)", address_cast(*cptr_cast<void**>(ptr)));
		case HSPVAR_FLAG_DOUBLE:  return strf((bShort ? "%f" : "%.16f"), *cptr_cast<double*>(ptr));
		case HSPVAR_FLAG_INT: {
			int const val = VtTraits::derefValptr<vtInt>(ptr);
#ifdef with_ModPtr
			assert(!ModPtr::isValid(val)); // addItem_value で処理されたはず
#endif
			return (bShort
				? strf("%d", val)
				: strf("%-10d (0x%08X)", val, val));
		}
		case HSPVAR_FLAG_LABEL:return nameFromLabel(VtTraits::derefValptr<vtLabel>(ptr));
		default: {
			auto const vtname = hpimod::getHvp(type)->vartype_name;

#ifdef with_ExtraBasics
			bool caught;
			auto&& exScalar = stringizeExtraScalar(vtname, ptr, bShort, caught);
			if ( caught ) return std::move(exScalar);
#endif
			return strf("unknown<%s>(0x%08X)", vtname, address_cast(ptr));
		}
	}
}

#ifdef with_ExtraBasics
string stringizeExtraScalar(char const* vtname, PDAT const* ptr, bool bShort, bool& caught)
{
	caught = true;
	bool bSigned = false;

	if ( strcmp(vtname, "bool") == 0 ) {
		static char const* const bool_name[2] = { "false", "true" };
		return bool_name[*cptr_cast<bool*>(ptr) ? 1 : 0];

		// char (signed char とする)
	} else if (
		(strcmp(vtname, "char") == 0 || strcmp(vtname, "signed_char") == 0) && (bSigned = true)
		|| (strcmp(vtname, "uchar") == 0 || strcmp(vtname, "unsigned_char") == 0)
		) {
		int const val = bSigned ? *cptr_cast<signed char*>(ptr) : *cptr_cast<unsigned char*>(ptr);
		return (val == 0) ? string("0 ('\\0')") : strf("%-3d '%c'", static_cast<int>(val), static_cast<char>(val));

		// short
	} else if (
		(strcmp(vtname, "short") == 0 || strcmp(vtname, "signed_short") == 0) && (bSigned = true)
		|| (strcmp(vtname, "ushort") == 0 || strcmp(vtname, "unsigned_short") == 0)
		) {
		int const val = static_cast<int>(bSigned ? *cptr_cast<signed short*>(ptr) : *cptr_cast<unsigned short*>(ptr));
		return (bShort ? strf("%d", val) : strf("%-6d (0x%04X)", val, static_cast<short>(val)));

		// unsigned int
	} else if ( strcmp(vtname, "uint") == 0 || strcmp(vtname, "unsigned_int") == 0 ) {
		auto const val = *cptr_cast<unsigned int*>(ptr);
		return (bShort ? strf("%d", val) : strf("%-10d (0x%08X)", val, val));

		// long
	} else if (
		(strcmp(vtname, "long") == 0 || strcmp(vtname, "signed_long") == 0) && (bSigned = true)
		|| (strcmp(vtname, "ulong") == 0 || strcmp(vtname, "unsigned_long") == 0)
		) {
		auto const   signed_val = *cptr_cast<long long*>(ptr);
		auto const unsigned_val = *cptr_cast<unsigned long long*>(ptr);
		return (bShort
			? strf("%d", (bSigned ? signed_val : unsigned_val))
			: strf("%d (0x%16X)", (bSigned ? signed_val : unsigned_val), signed_val)
			);
		// おまけ
	} else if ( strcmp(vtname, "tribyte") == 0 ) {
		auto const bytes = cptr_cast<char*>(ptr);
		int const val = bytes[0] << 16 | bytes[1] << 8 | bytes[2];
		return (bShort
			? strf("%d", val)
			: strf("%-8d (0x%06X)", val, val)
			);
	} else {
		caught = false;
		return string();
	}
}
#endif

//------------------------------------------------
// モジュールクラス名を表す文字列
//------------------------------------------------
string nameFromModuleClass(stdat_t stdat, bool bClone)
{
	auto const modclsName = hpimod::STRUCTDAT_getName(stdat);
	return (bClone
		? strf("%s&", modclsName)
		: modclsName);
}

//------------------------------------------------
// 構造体パラメータの名前
// 
// デバッグ情報から取得する。なければ「(idx)」とする。
//------------------------------------------------
string nameFromStPrm(stprm_t stprm, int idx)
{
	int const subid = hpimod::findStPrmIndex(stprm);
	if ( subid >= 0 ) {
		if ( auto const name = g_dbginfo->getAx().tryFindParamName(subid) ) {
			return hpimod::nameExcludingScopeResolution(name);

		// thismod 引数
		} else if ( stprm->mptype == MPTYPE_MODULEVAR || stprm->mptype == MPTYPE_IMODULEVAR || stprm->mptype == MPTYPE_TMODULEVAR ) {
			return "thismod";
		}
	}
	return hpimod::stringizeArrayIndex({ idx });
}

//------------------------------------------------
// ラベルの名前
// 
// デバッグ情報から取得する。なければ「label(address)」とする。
//------------------------------------------------
string nameFromLabel(label_t lb)
{
	int const otIndex = hpimod::findOTIndex(lb);
	if ( auto const name = g_dbginfo->getAx().tryFindLabelName(otIndex) ) {
		return strf("*%s", name);
	} else {
		return strf("label(0x%08X)", address_cast(lb));
	}
}
