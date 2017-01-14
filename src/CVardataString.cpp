// 変数データのツリー形式文字列

#include <algorithm>
#include "module/ptr_cast.h"
#include "module/CStrWriter.h"
#include "module/CStrBuf.h"
#include "hpiutil/hpiutil.hpp"

#include "main.h"
#include "CVardataString.h"
#include "CVarinfoText.h"
#include "config_mng.h"

using namespace hpiutil::internal_vartype_tags;

CVardataStrWriter::CVardataStrWriter(CVardataStrWriter&& src)
	: writer_(std::move(src.writer_))
{}

CVardataStrWriter::~CVardataStrWriter()
{}

auto CVardataStrWriter::getString() const -> string const&
{
	return getWriter().get();
}

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
	assert(!! pval);

	if ( auto addVar = static_cast<addVarUserdef_t>(g_config->vswInfo[pval->flag].addVar) ) {
		return addVar(this, name, pval);
	}
	
	if ( hpiutil::PVal_isStandardArray(pval) ) {
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
	addValue(name, pval->flag, hpiutil::PVal_getPtr(pval));
}

void CVardataStrWriter::addVarScalar(char const* name, PVal const* pval, APTR aptr)
{
	addValue(name, pval->flag, hpiutil::PVal_getPtr(pval, aptr));
}

//------------------------------------------------
// [add][item] 標準配列変数
//------------------------------------------------
void CVardataStrWriter::addVarArray(char const* name, PVal const* pval)
{
	auto const hvp = hpiutil::varproc(pval->flag);
	auto const cntElem = hpiutil::PVal_cntElems(pval);

	getWriter().catNodeBegin(name, strf("<%s>[", hvp->vartype_name).c_str());

	// ツリー状文字列の場合
	if ( ! getWriter().isLineformed() ) {
		getWriter().catAttribute("type", stringizeVartype(pval).c_str());

		for ( auto i = size_t { 0 }; i < cntElem; ++i ) {
			auto indexes = hpiutil::PVal_indexesFromAptr(pval, i);

			// 要素の値を追加
			auto nameChild = hpiutil::stringifyArrayIndex(indexes);
			addVarScalar(nameChild.c_str(), pval, i);
		}

	// 一行文字列の場合
	} else if ( cntElem > 0 ) {
		auto const dim = hpiutil::PVal_maxDim(pval);

		// cntElems[1 + i] = 部分i次元配列の要素数
		// (例えば配列 int(2, 3, 4) だと、cntElems = {1, 2, 2*3, 2*3*4, 0})
		auto cntElems = indexes_t { {1} };
		for ( auto i = size_t { 0 }; i < dim && pval->len[i + 1] > 0; ++i ) {
			cntElems[i + 1] = pval->len[i + 1] * cntElems[i];
		}

		addVarArrayRec(pval, cntElems, dim - 1, 0);
	}

	getWriter().catNodeEnd("]");
}

void CVardataStrWriter::addVarArrayRec(PVal const* pval, indexes_t const& cntElems, size_t idxDim, APTR aptr_offset)
{
	assert(getWriter().isLineformed());
	for ( auto i = 0; i < pval->len[idxDim + 1]; ++i ) {
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

	if ( auto addValue = static_cast<addValueUserdef_t>(g_config->vswInfo[type].addValue) ) {
		return addValue(this, name, ptr);
	}

	auto addValueLeaf = [&](string const& repr) {
		getWriter().catLeaf(name, repr.c_str());
	};

	hpiutil::dispatchValue<void>(ptr, type
		, [&](label_t lb)          { addValueLeaf(hpiutil::nameFromLabel(lb)); }
		, [&](char const* str)     { addValueString(name, str); }
		, [&](double val) {
				addValueLeaf(strf((getWriter().isLineformed() ? "%f" : "%.16f"), val));
			}
		, [&](int val) {
				addValueLeaf((getWriter().isLineformed()
					? strf("%d", val)
					: strf("%-10d (0x%08X)", val, val)
					));
			}
		, [&](FlexValue const& fv) { addValueStruct(name, &fv); }
		, [&](PDAT const* p, vartype_t vtype) {
				auto const vtname = hpiutil::varproc(type)->vartype_name;
				addValueLeaf(strf("unknown<%s>(%p)", vtname, cptr_cast<void*>(ptr)));
			}
		);
}

//------------------------------------------------
// [add][item] string
//------------------------------------------------
void CVardataStrWriter::addValueString(char const* name, char const* str)
{
	if ( getWriter().isLineformed() ) {
		getWriter().catLeaf(name, hpiutil::literalFormString(str).c_str());

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

	if ( ! fv->ptr || fv->type == FLEXVAL_TYPE_NONE ) {
		getWriter().catLeafExtra(name, "nullmod");

	} else {
		auto const stdat = hpiutil::FlexValue_module(fv);
		auto const modclsNameString =
			hpiutil::nameFromModuleClass(stdat, hpiutil::FlexValue_isClone(fv));

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
	assert(!! prmstk.first);
	auto prev_mptype = MPTYPE_NONE;
	auto i = 0;

	for ( auto& stprm : hpiutil::STRUCTDAT_params(stdat) ) {
		auto const member = hpiutil::Prmstack_memberPtr(prmstk.first, &stprm);

		// if treeformed: put an additional ' ' before 'local' parameters
		if ( ! getWriter().isLineformed()
			&& i > 0
			&& (prev_mptype != MPTYPE_LOCALVAR && stprm.mptype == MPTYPE_LOCALVAR)
			) {
			getWriter().cat(" ");
		}

		addParameter(hpiutil::nameFromStPrm(&stprm, i).c_str(), stdat, &stprm, member, prmstk.second);

		// structtag isn't a member
		if ( stprm.mptype != MPTYPE_STRUCTTAG ) { ++i; }
	}
}

//------------------------------------------------
// [add][item] メンバ (in prmstack)
//------------------------------------------------
void CVardataStrWriter::addParameter(char const* name, stdat_t stdat, stprm_t stprm, void const* member, bool isSafe)
{
	if ( ! isSafe ) {
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
			auto const fv = hpiutil::asValptr<vtStruct>(hpiutil::PVal_getPtr(thismod->pval, thismod->aptr));
			addValueStruct(name, fv);
			break;
		}
		//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
			addValue(name, HSPVAR_FLAG_STR, hpiutil::asPDAT<vtStr>(*cptr_cast<char**>(member)));
			break;

		case MPTYPE_DNUM:
			addValue(name, HSPVAR_FLAG_DOUBLE, hpiutil::asPDAT<vtDouble>(cptr_cast<double*>(member)));
			break;

		case MPTYPE_INUM:
			addValue(name, HSPVAR_FLAG_INT, hpiutil::asPDAT<vtInt>(cptr_cast<int*>(member)));
			break;

		case MPTYPE_LABEL:
			addValue(name, HSPVAR_FLAG_LABEL, hpiutil::asPDAT<vtLabel>(cptr_cast<label_t*>(member)));
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
void CVardataStrWriter::addSysvar(hpiutil::Sysvar::Id id)
{
	using namespace hpiutil;

	auto name = Sysvar::List[id].name;

	switch ( id ) {
		case Sysvar::Id::Refstr:
			addValue(name, HSPVAR_FLAG_STR, hpiutil::asPDAT<vtStr>(ctx->refstr));
			break;

		case Sysvar::Id::Refdval:
			addValue(name, HSPVAR_FLAG_DOUBLE, hpiutil::asPDAT<vtDouble>(&ctx->refdval));
			break;

		case Sysvar::Id::Cnt:
			if ( ctx->looplev > 0 ) {
				// int 配列と同じ表示にする
				getWriter().catNodeBegin(name, "<int>[");
				for ( int i = 0; i < ctx->looplev; ++ i ) {
					auto const lvLoop = ctx->looplev - i;
					addValue(strf("#%d", lvLoop).c_str()
						, HSPVAR_FLAG_INT
						, hpiutil::asPDAT<vtInt>(&ctx->mem_loop[lvLoop].cnt)
						);
				}
				getWriter().catNodeEnd("]");
			} else {
				getWriter().catLeafExtra(name, "out_of_loop");
			}
			break;

		case Sysvar::Id::NoteBuf:
		{
			if ( auto const pval = ctx->note_pval ) {
				auto const aptr = ctx->note_aptr;

				auto indexes     = hpiutil::PVal_indexesFromAptr(pval, aptr);
				auto indexString = hpiutil::stringifyArrayIndex(indexes);
				auto varName     = hpiutil::nameFromStaticVar(pval);
				auto name2 =
					varName
					? strf("%s (%s%s)", name, varName, indexString)
					: strf("%s (%p (%d))", name, cptr_cast<void*>(pval), aptr);
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
				addValue(name, HSPVAR_FLAG_INT, hpiutil::asPDAT<vtInt>(&Sysvar::getIntRef(id)));
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
	auto name = hpiutil::STRUCTDAT_name(stdat);

	getWriter().catNodeBegin(name, strf("%s(", name).c_str());
	if ( ! prmstk.first ) {
		getWriter().catLeafExtra("arguments", "not_available");
	} else {
		addPrmstack(stdat, prmstk);
	}
	getWriter().catNodeEnd(")");
}

void CVardataStrWriter::addResult(stdat_t stdat, PDAT const* resultPtr, vartype_t resultType)
{
	assert(!! resultPtr);
	auto name = hpiutil::STRUCTDAT_name(stdat);

	getWriter().catNodeBegin(name, strf("%s => ", name).c_str());
	addValue(".result", resultType, resultPtr);
	getWriter().catNodeEnd("");
}
