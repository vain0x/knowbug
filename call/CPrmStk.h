// prmstk クラス

#ifndef IG_CLASS_PARAMETER_STACK_CREATOR_MANAGED_H
#define IG_CLASS_PARAMETER_STACK_CREATOR_MANAGED_H

#include <vector>

#include "hsp3plugin_custom.h"
#include "CPrmInfo.h"
#include "CPrmStkCreator.h"
#include "ManagedPVal.h"

#include "../var_vector/vt_vector.h"

/**
@summary:
	HSP の prmstk と同じ形式でデータを格納するコンテナ。

	prmstk バッファ、str 引数、値渡しされた any 引数用の PVal、flex 用の vector、local 変数を管理する機能を持つ。
**/

//------------------------------------------------
// 高級な prmstk 
//------------------------------------------------
class CPrmStk
	: private CPrmStkCreator
{
private:
	CPrmInfo const& prminfo_;

	// 現在、追加された実引数の個数 (flex は除く)
	size_t cntArgs_;

public:
	CPrmStk(CPrmInfo const& prminfo)
		: CPrmStkCreator(hspmalloc(prminfo.getStackSize()), prminfo.getStackSize())
		, prminfo_ { prminfo }
	{ }

	~CPrmStk()
	{
		void* const prmstk = getptr();
		assert(!!prmstk);

		// スタック上の管理されたオブジェクトを解体する
		// todo: reinterpret_cast を使わなくていいように
		for ( size_t i = 0; i < prminfo_.cntPrms(); ++i ) {
			int const prmtype = prminfo_.getPrmType(i);
			size_t const offset = prminfo_.getStackOffset(i);

			switch ( prmtype ) {
				case HSPVAR_FLAG_STR:
				{
					auto const p = reinterpret_cast<char**>(getPtrAt(i));
					assert(!!p);
					hspfree(*p);
					break;
				}
				case PRM_TYPE_ANY:
				{
					auto const vardat = reinterpret_cast<MPVarData*>(getPtrAt(i));
					auto&& pval = ManagedPVal::ofValptr(vardat.pval);
					pval.decRef();
					break;
				}
				case PRM_TYPE_LOCAL:
				{
					auto const pval = reinterpret_cast<PVal*>(getPtrAt(i));
					PVal_free(pval);
					break;
				}
			}
		}
		if ( void* const pFlex = getPtrFlex() ) {
			vector_t&& vec = vector_t::ofValptr(p);
			vec.decRef();
		}

		hspfree(prmstk);
	}

	void* getPtr() const { return CPrmStkCreator::getptr(); }
	size_t cntArgs() const { return cntArgs_; }

public:
	//------------------------------------------------
	// 実引数値の push 各種
	//------------------------------------------------

	// 単純な値
	template<typename VartypeTag>
	void pushValue(PDAT const* pdat)
	{
		pushValue<typename VtTraits<VartypeTag>::value_t>(VtTraits<VartypeTag>::derefValptr(pdat));

		++cntArgs_;
	}

	// 文字列 (prmstk 用にコピーを取る)
	void pushString(char const* src)
	{
		char** p = allocValue<char*>();
		size_t const len = std::strlen(src);
		size_t const size = len * sizeof(char) + 1;
		*p = hspmalloc(size);
		strcpy_s(*p, len * size, src);

		++cntArgs_;
		return;
	}

	// 値渡しの any (PVal* に保存する)
	void pushAnyByVal(PDAT const* pdat, hpimod::vartype_t vtype)
	{
		hpimod::ManagedPVal pval;
		hpimod::PVal_assign(pval.valuePtr(), pdat, vtype);
		pushPVal(pval.valuePtr(), 0);
		pval.incRef();	// prmstk による所有

		++cntArgs_;
		return;
	}

	// 参照渡しの any (管理の必要はない)
	void pushAnyByRef(PVal* pval, APTR aptr)
	{
		pushPVal(pval, aptr);
		++cntArgs_;
		return;
	}

	// 可変長引数
	vector_t& pushFlex()
	{
		assert(prminfo_.cntPrms() == cntArgs());
		vector_t* const vec = allocValue<vector_t>();
		new(vec)vector_t();
		return *vec;
	}

	// push local
	PVal* pushLocal() // shadowing
	{
		PVal* const pval = CPrmStkCreator::allocLocal();
		hpimod::PVal_init(pval, HSPVAR_FLAG_INT);
		return pval;
	}

	// push local all
	PVal* pushLocalAll()
	{
		for ( size_t i = 0; i < prminfo_.cntLocals(); ++i ) {
			pushLocal();
		}
	}

	// push (prmtype) byVal
	void pushArgByVal(PDAT const* pdat, vartype_t vtype)
	{
		int const prmtype = prminfo_.getPrmType(cntArgs());

		switch ( prmtype ) {
			case HSPVAR_FLAG_LABEL:  pushValue<label_t>(pdat); break;
			case HSPVAR_FLAG_DOUBLE: pushValue<double >(pdat); break;
			case HSPVAR_FLAG_INT:    pushValue<int    >(pdat); break;
			case HSPVAR_FLAG_STR:
				pushString(VtTraits<str_tag>::derefValptr(pdat));
				break;

			case PRM_TYPE_ANY: pushAnyByVal(pdat, vtype); break;

			default:
				// その他の型タイプ値
				if ( HSPVAR_FLAG_INT < prmtype && prmtype < (HSPVAR_FLAG_USERDEF + ctx->hsphed->max_varhpi) ) {
					if ( vtype != prmtype ) puterror(HSPERR_TYPE_MISMATCH);
					pushAnyByVal(pdat, vtype);
					break;
				}

				// 異常
				puterror(
					(PrmType_IsRef(prmtype)
						? HSPERR_VARIABLE_REQUIRED
						: HSPERR_ILLEGAL_FUNCTION)
				);
		}
	}

	// push (prmtype) byRef
	void pushArgByRef(PVal* pval, APTR aptr)
	{
		int const prmtype = prminfo_.getPrmType(cntArgs());

		switch ( prmtype ) {
			case PRM_TYPE_VAR:
			case PRM_TYPE_ARRAY:
			case PRM_TYPE_ANY:
				pushPVal(pval, aptr);
				break;

			case PRM_TYPE_MODVAR:
			{
				auto const fv = VtTraits<struct_tag>::getValptr(pval);
				pushThismod(pval, aptr, FlexValue_getModuleTag(fv)->subid);
				break;
			}
			default:
				puterror(HSPERR_VARIABLE_REQUIRED);
		}

	}

	// push arg byDefault
	PVal* pushArgDefault() const
	{
		size_t const idx = cntArgs();
		assert( idx < prminfo_.cntPrms() );

		int const prmtype = prminfo_.getPrmType(idx);

		switch ( prmtype ) {
			case HSPVAR_FLAG_LABEL:  puterror(HSPERR_LABEL_REQUIRED);
			case HSPVAR_FLAG_STRUCT: puterror(HSPERR_STRUCT_REQUIRED);

			case PRM_TYPE_ANY:
			{
				static int const zero = 0;
				return pushArgByVal(VtTraits<int>::asValptr(&zero), HSPVAR_FLAG_INT);
			}
			default:
				// 型タイプ値の引数 => その型の既定値
				if ( HSPVAR_FLAG_NONE < prmtype && prmtype < (HSPVAR_FLAG_USERDEF + ctx->hsphed->max_varhpi) ) {
					if ( vtype != prmtype ) puterror(HSPERR_TYPE_MISMATCH);
					PVal* const pvalDefault = hpimod::PVal_getDefault(prmtype);
					
					pushPrmByVal(prmtype, pvalDefault->pt, pvalDefault->flag);
					break;
				}

				puterror(
					(PrmType_IsRef(prmtype) ? HSPERR_VARIABLE_REQUIRED : HSPERR_NO_DEFAULT)
				);
				throw;
		}
	}

	//------------------------------------------------
	// 実引数値の peek 各種
	//------------------------------------------------

	void* getPtrAt(size_t idx) const
	{
		assert(idx < cntArgs());
		return &reinterpret_cast<char*>(getPtr())[ prminfo_.getStackOffset(idx) ];
	}
	void* getPtrFlex() const
	{
		return ( prminfo_.isFlex() )
			? &reinterpret_cast<char*>(getPtr())[ prminfo_.getStackSize() - sizeof(vector_t) ]
			: nullptr;
	}

	/*
	PDAT* peekValue(size_t idx, vartype_t& vtype)
	{
		assert(idx < cntArgs());

		int const prmtype = prminfo_.getPrmType(idx);
		switch ( prmtype ) {
			//todo: implement
		}
		return;
	}//*/
};

#endif
