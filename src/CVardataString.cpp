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
#include "config_mng.h"

#include "with_ModPtr.h"

namespace VtTraits { using namespace hpimod::VtTraits; }
using namespace hpimod::VtTraits::InternalVartypeTags;

static string stringizeSimpleValue(vartype_t type, PDAT const* ptr, bool bShort);
static string nameFromModuleClass(stdat_t stdat, bool bClone);
static string nameFromStPrm(stprm_t stprm, int idx);
static string nameFromLabel(label_t lb);

CVardataStrWriter::CVardataStrWriter(CVardataStrWriter&& src) : writer_(std::move(src.writer_)) {}
CVardataStrWriter::~CVardataStrWriter() {}

string const& CVardataStrWriter::getString() const { return getWriter().get(); }

//------------------------------------------------
// 枝刈りを試みる
// この関数は ptr が指す何らかのデータを探索する直前にだけ呼ばれる。
// ptr が探索済みなら値の表示を省略し、true を返す。
//------------------------------------------------
bool CVardataStrWriter::tryPrune(char const* name, void const* ptr) const
{
	if ( visited_.count(ptr) != 0 ) {
		auto& ref_name = visited_[ptr];
		getWriter().catLeafExtra(name, strf("(ref: %s)", ref_name).c_str());
		return true;

	} else {
		visited_.emplace(ptr, name);
		return false;
	}
}

//------------------------------------------------
// [add][item] 変数
//------------------------------------------------
void CVardataStrWriter::addVar(char const* name, PVal const* pval)
{
	assert(!!pval);

	if ( addVarUserdef_t const addVar = g_config->vswInfo[pval->flag].addVar ) {
		return addVar(this, name, pval);
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

	if ( addValueUserdef_t const addValue = g_config->vswInfo[type].addValue ) {
		addValue(this, name, ptr);
		return;
	}

	if ( type == HSPVAR_FLAG_STRUCT ) {
		addValueStruct(name, VtTraits::asValptr<vtStruct>(ptr));

	} else if ( type == HSPVAR_FLAG_STR ) {
		addValueString(name, VtTraits::asValptr<vtStr>(ptr));

	} else {
		auto const&& dbgstr = stringizeSimpleValue(type, ptr, getWriter().isLineformed());
		getWriter().catLeaf(name, dbgstr.c_str());
	}
}

//------------------------------------------------
// [add][item] string
//------------------------------------------------
void CVardataStrWriter::addValueString(char const* name, char const* str)
{
	if ( getWriter().isLineformed() ) {
		getWriter().catLeaf(name, hpimod::literalFormString(str).c_str());

	} else {
		if ( strstr(str, "\r\n") ) {
			getWriter().catNodeBegin(name, CLineformedWriter::stc_strUnused);
			getWriter().cat(str);
			getWriter().catNodeEnd(CLineformedWriter::stc_strUnused);
		} else {
			getWriter().catLeaf(name, str);
		}
	}
}

//------------------------------------------------
// [add][item] flex-value
//------------------------------------------------
void CVardataStrWriter::addValueStruct(char const* name, FlexValue const* fv)
{
	if ( tryPrune(name, fv) ) return;
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
	assert(!!prmstk.first);
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
				string&& name2 = (varName
					? strf("%s (%s%s)", name, varName, indexString)
					: strf("%s (%p (%d))", name, cptr_cast<void*>(pval), aptr));
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

	getWriter().catNodeBegin(name, strf("%s(", name).c_str());
	if ( !prmstk.first ) {
		getWriter().catLeafExtra("arguments", "not_available");
	} else {
		addPrmstack(stdat, prmstk);
	}
	getWriter().catNodeEnd(")");
}

void CVardataStrWriter::addResult(stdat_t stdat, PDAT const* resultPtr, vartype_t resultType)
{
	assert(!!resultPtr);
	char const* const name = hpimod::STRUCTDAT_getName(stdat);

	getWriter().catNodeBegin(name, strf("%s => ", name).c_str());
	addValue(".result", resultType, resultPtr);
	getWriter().catNodeEnd("");
}

//------------------------------------------------
// 単純な値を文字列化する
//
// @ addItem_value でフックされる型はここに来ない。
//------------------------------------------------
string stringizeSimpleValue(vartype_t type, PDAT const* ptr, bool bShort)
{
	assert(ptr);

	switch ( type ) {
		case HSPVAR_FLAG_STR:
		case HSPVAR_FLAG_INT:
		case HSPVAR_FLAG_STRUCT: assert_sentinel;

		case HSPVAR_FLAG_COMOBJ:  return strf("comobj(%p)", *cptr_cast<void**>(ptr));
		case HSPVAR_FLAG_VARIANT: return strf("variant(%p)", *cptr_cast<void**>(ptr));
		case HSPVAR_FLAG_DOUBLE:  return strf((bShort ? "%f" : "%.16f"), *cptr_cast<double*>(ptr));

		case HSPVAR_FLAG_LABEL:return nameFromLabel(VtTraits::derefValptr<vtLabel>(ptr));
		default: {
			auto const vtname = hpimod::getHvp(type)->vartype_name;
			return strf("unknown<%s>(%p)", vtname, cptr_cast<void*>(ptr));
		}
	}
}

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
		return strf("label(%p)", cptr_cast<void*>(lb));
	}
}
