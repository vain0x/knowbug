// 仮引数情報クラス

#include "mod_makepval.h"

#include "CPrmInfo.h"

using namespace hpimod;

CPrmInfo const CPrmInfo::undeclaredFunc = CPrmInfo( nullptr, true );
CPrmInfo const CPrmInfo::noprmFunc     = CPrmInfo( nullptr, false );

//##############################################################################
//                定義部 : CPrmInfo
//##############################################################################
//------------------------------------------------
// 標準構築
//------------------------------------------------
CPrmInfo::CPrmInfo(prmlist_t const* pPrmlist, bool bFlex)
	: mcntPrms( 0 )
	, mcntLocals( 0 )
	, mbFlex  ( bFlex )
{
	if ( pPrmlist ) {
		setPrmlist( *pPrmlist );	// prmlist の複写
	}
	return;
}

//###############################################
//    設定系
//###############################################
//-----------------------------------------------
// 可変長引数か
//-----------------------------------------------
void CPrmInfo::setFlex( bool bFlex )
{
	mbFlex = bFlex;
	return;
}

//------------------------------------------------
// prmlist の複写
//------------------------------------------------
void CPrmInfo::setPrmlist( prmlist_t const& prmlist )
{
	if ( prmlist.empty() ) return;

	mcntPrms   = 0;
	mcntLocals = 0;
	mprmlist.clear();
	mprmlist.reserve( mcntPrms );

	for ( auto& it : prmlist ) {
		if ( it == PRM_TYPE_FLEX ) {
			mbFlex = true;

		} else if ( it == PRM_TYPE_LOCAL ) {
			mcntLocals ++;

		} else {
			mprmlist.push_back( it );
			mcntPrms ++;
		}
	}

	return;
}

//###############################################
//    取得系
//###############################################
//-----------------------------------------------
// 引数の数
//-----------------------------------------------
size_t CPrmInfo::cntPrms() const
{
	return mcntPrms;
}

//-----------------------------------------------
// ローカル変数の数
//-----------------------------------------------
size_t CPrmInfo::cntLocals() const
{
	return mcntLocals;
}

//-----------------------------------------------
// 可変長引数か
//-----------------------------------------------
bool CPrmInfo::isFlex() const
{
	return mbFlex;
}

//-----------------------------------------------
// 仮引数タイプ
// 
// @ 失敗すると PRM_TYPE_NONE を返す。
//-----------------------------------------------
int CPrmInfo::getPrmType( size_t index ) const
{
	if ( index < 0 ) return PRM_TYPE_NONE;
	if ( index >= mcntPrms ) {
		return ( isFlex() ) ? PRM_TYPE_ANY : PRM_TYPE_NONE;	// 可変部分 or 過剰
	}

	return mprmlist[index];
}

//-----------------------------------------------
// スタックサイズ計算
// 
// @ 可変長引数は無視する。
//-----------------------------------------------
int CPrmInfo::getStackSize() const
{
	int sum = 0;

	for ( size_t i = 0; i < mcntPrms; ++ i ) {
		sum += PrmType_Size( getPrmType(i) );
	}

	sum += PrmType_Size( PRM_TYPE_LOCAL ) * mcntLocals;
	return sum;
}

// 可変長引数込み (cntFlex: 可変長部分の個数)
int CPrmInfo::getStackSizeWithFlex( size_t cntFlex ) const
{
	return getStackSize() + (PrmType_Size(PRM_TYPE_ANY) * cntFlex);
}

//-----------------------------------------------
// 正しい引数か否か
//-----------------------------------------------
void CPrmInfo::checkCorrectArg( PVal const* pvArg, size_t iArg, bool bByRef ) const
{
	int const prmtype = getPrmType(iArg);

	// 可変長引数
	if ( iArg >= cntPrms() ) {
		if ( isFlex() ) {
			// 必ず正しいことにする
		} else {
			puterror( HSPERR_TOO_MANY_PARAMETERS );
		}

	// any
	} else if ( prmtype == PRM_TYPE_ANY ) {
		// OK

	// 参照渡し要求
	} else if ( prmtype == PRM_TYPE_VAR || prmtype == PRM_TYPE_ARRAY ) {
		if ( !bByRef ) {
			puterror( HSPERR_VARIABLE_REQUIRED );
		}

	// 型不一致
	} else if ( prmtype != pvArg->flag ) {
		puterror( HSPERR_TYPE_MISMATCH );
	}

	return;
}

//-----------------------------------------------
// 省略値を取得
// 
// @ 省略できない => エラー
//-----------------------------------------------
PVal* CPrmInfo::getDefaultArg( size_t iArg ) const
{
	// 可変長引数
	if ( iArg >= cntPrms() ) {
		if ( isFlex() ) {
			return PVal_getDefault();
		} else {
			puterror( HSPERR_TOO_MANY_PARAMETERS );
		}
	}

	int const prmtype = getPrmType(iArg);

	switch ( prmtype ) {
		// 通常仮引数で、既定値のある型
		case HSPVAR_FLAG_STR:
		case HSPVAR_FLAG_DOUBLE:
		case HSPVAR_FLAG_INT:
			return PVal_getDefault( prmtype );

		// 通常仮引数で、省略不可
		case HSPVAR_FLAG_LABEL:  puterror( HSPERR_LABEL_REQUIRED );
		case HSPVAR_FLAG_STRUCT: puterror( HSPERR_STRUCT_REQUIRED );

		// any
		case PRM_TYPE_ANY:
			return PVal_getDefault();

		default:
			// 参照渡し要求
			if ( PrmType_IsRef(prmtype) ) puterror( HSPERR_VARIABLE_REQUIRED );

			// その他
			puterror( HSPERR_NO_DEFAULT );
	}

	throw;	// 警告抑制
}

//################################################
//    演算子
//################################################
//------------------------------------------------
// 比較
//------------------------------------------------
int CPrmInfo::compare( CPrmInfo const& rhs ) const
{
	if ( mbFlex   != rhs.mbFlex   ) return (mbFlex   ? 1 : -1);
	if ( mcntPrms != rhs.mcntPrms ) return mcntPrms - rhs.mcntPrms;

	for ( size_t i = 0; i < mcntPrms; ++ i ) {
		int const diff = getPrmType(i) - rhs.getPrmType(i);
		if ( diff ) return diff;
	}
	return 0;
}

//################################################
//    内部メンバ関数
//################################################
//------------------------------------------------
// 複写
//------------------------------------------------
CPrmInfo& CPrmInfo::copy( CPrmInfo const& src )
{
	mcntPrms   = src.mcntPrms;
	mcntLocals = src.mcntLocals;
	mbFlex     = src.mbFlex;
	mprmlist   = src.mprmlist;
	return *this;
}

//------------------------------------------------
// CPrmInfo <- STRUCTDAT
//------------------------------------------------
CPrmInfo CPrmInfo::Create(stdat_t stdat)
{
	CPrmInfo::prmlist_t prmlist;
	prmlist.reserve(stdat->prmmax);

	stprm_t const stprm = hpimod::STRUCTDAT_getStPrm(stdat);

	for ( int i = 0; i < stdat->prmmax; ++i ) {
		int const prmtype = PrmType_FromMPType(stprm[i].mptype);
		if ( prmtype != PRM_TYPE_NONE ) {
			prmlist.push_back(prmtype);
		}
	}
	return CPrmInfo(&prmlist, false);
}

//##############################################################################
//                その他の関数
//##############################################################################
//------------------------------------------------
// 参照渡しの仮引数タイプか
//------------------------------------------------
bool PrmType_IsRef(int prmtype)
{
	return prmtype == PRM_TYPE_VAR
		|| prmtype == PRM_TYPE_ARRAY
		|| prmtype == PRM_TYPE_MODVAR
		;
}

//------------------------------------------------
// 仮引数タイプが prmstack に要求するサイズ
//------------------------------------------------
int PrmType_Size(int prmtype)
{
	switch ( prmtype ) {
		case HSPVAR_FLAG_LABEL:  return sizeof(label_t);
		case HSPVAR_FLAG_STR:    return sizeof(char*);
		case HSPVAR_FLAG_DOUBLE: return sizeof(double);
		case HSPVAR_FLAG_INT:    return sizeof(int);
		case PRM_TYPE_VAR:
		case PRM_TYPE_ARRAY:
		case PRM_TYPE_ANY:    return sizeof(MPVarData);
		case PRM_TYPE_MODVAR: return sizeof(MPModVarData);
		case PRM_TYPE_LOCAL:  return sizeof(PVal);
		default:
			// その他の型タイプ値
			if ( HSPVAR_FLAG_INT < prmtype && prmtype < (HSPVAR_FLAG_USERDEF + ctx->hsphed->max_varhpi) ) {
				return sizeof(MPVarData);
			}
			return 0;
	}
}

//------------------------------------------------
// prmtype <- mptype
//------------------------------------------------
int PrmType_FromMPType(int mptype)
{
	switch ( mptype ) {
		case MPTYPE_LABEL:       return HSPVAR_FLAG_LABEL;
		case MPTYPE_LOCALSTRING: return HSPVAR_FLAG_STR;
		case MPTYPE_DNUM:        return HSPVAR_FLAG_DOUBLE;
		case MPTYPE_INUM:        return HSPVAR_FLAG_INT;
		case MPTYPE_SINGLEVAR:   return PRM_TYPE_VAR;
		case MPTYPE_ARRAYVAR:    return PRM_TYPE_ARRAY;
		case MPTYPE_TMODULEVAR:  return PRM_TYPE_FLEX; // destructor は可変長引数ということにする (modcls で使うため)
		case MPTYPE_IMODULEVAR:	//
		case MPTYPE_MODULEVAR:   return PRM_TYPE_MODVAR;
		case MPTYPE_LOCALVAR:    return PRM_TYPE_LOCAL;

		default:
			return PRM_TYPE_NONE;
	}
}
