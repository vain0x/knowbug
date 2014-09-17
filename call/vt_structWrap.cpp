// Call(ModCls) - vartype struct

#include <map>
#include <array>
#include <limits>

#include "hsp3plugin_custom.h"
#include "mod_makePVal.h"

#include "cmd_modcls.h"
#include "vt_structWrap.h"
#include "modcls_FlexValue.h"

#include "CCaller.h"
#include "Functor.h"
#include "CPrmInfo.h"
#include "vt_functor.h"

using namespace hpimod;
using namespace ModCls;

// テンポラリ変数への参照 (普通には参照できないので、与えられたときに保存する)
PVal* mpval_struct = nullptr;

PVal* ModCls::getMPValStruct()
{
	return mpval_struct;
}
PVal* ModCls::getMPVal(vartype_t type)
{
	return (mpval_struct ? (mpval_struct - HSPVAR_FLAG_STRUCT + type) : nullptr);
}

// 元々の struct 型
static HspVarProc hvp_struct_impl;

// 前方宣言
static void HspVarStructWrap_Alloc( PVal *pval, const PVal *pval2 );
static void HspVarStructWrap_Free( PVal *pval );

static void* HspVarStructWrap_Cnv      ( void const* buffer, int flag );
static void* HspVarStructWrap_CnvCustom( void const* buffer, int flag );

static void HspVarStructWrap_Set( PVal *pval, PDAT *pdat, const void *in );

static void HspVarStructWrap_AddI ( PDAT* pdat, void const* val );
static void HspVarStructWrap_SubI ( PDAT* pdat, void const* val );
static void HspVarStructWrap_MulI ( PDAT* pdat, void const* val );
static void HspVarStructWrap_DivI ( PDAT* pdat, void const* val );
static void HspVarStructWrap_ModI ( PDAT* pdat, void const* val );

static void HspVarStructWrap_AndI ( PDAT* pdat, void const* val );
static void HspVarStructWrap_OrI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_XorI ( PDAT* pdat, void const* val );

static void HspVarStructWrap_EqI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_NeI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_GtI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_LtI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_GtEqI( PDAT* pdat, void const* val );
static void HspVarStructWrap_LtEqI( PDAT* pdat, void const* val );

static void HspVarStructWrap_RrI  ( PDAT* pdat, void const* val );
static void HspVarStructWrap_LrI  ( PDAT* pdat, void const* val );

static void HspVarStructWrap_Method( PVal* pval );

static void HspVarStructWrap_InitCnvWrap();

//------------------------------------------------
// struct 処理関数を置き換える
//------------------------------------------------
void HspVarStructWrap_Init( HspVarProc* vp )
{
	hvp_struct_impl = *vp;

	vp->Alloc = HspVarStructWrap_Alloc;
	vp->Free  = HspVarStructWrap_Free;

	vp->Cnv   = HspVarStructWrap_Cnv;

	vp->Set   = HspVarStructWrap_Set;

	vp->AddI  = HspVarStructWrap_AddI;
	vp->SubI  = HspVarStructWrap_SubI;
	vp->MulI  = HspVarStructWrap_MulI;
	vp->DivI  = HspVarStructWrap_DivI;
	vp->ModI  = HspVarStructWrap_ModI;

	vp->AndI  = HspVarStructWrap_AndI;
	vp->OrI   = HspVarStructWrap_OrI;
	vp->XorI  = HspVarStructWrap_XorI;

	vp->EqI   = HspVarStructWrap_EqI;
	vp->NeI   = HspVarStructWrap_NeI;
	vp->GtI   = HspVarStructWrap_GtI;
	vp->LtI   = HspVarStructWrap_LtI;
	vp->GtEqI = HspVarStructWrap_GtEqI;
	vp->LtEqI = HspVarStructWrap_LtEqI;

	vp->RrI   = HspVarStructWrap_RrI;
	vp->LrI   = HspVarStructWrap_LrI;

	vp->ObjectMethod = HspVarStructWrap_Method;

	HspVarStructWrap_InitCnvWrap();
	return;
};

//------------------------------------------------
// 確保
//
// @ 初期値は nullmod とする。
//------------------------------------------------
void HspVarStructWrap_Alloc( PVal* pval, PVal const* pval2 )
{
	if ( pval->len[1] < 1 ) pval->len[1] = 1;		// 配列を最低1は確保する

	size_t const size = sizeof(FlexValue) * pval->len[1];
	auto const pt = reinterpret_cast<FlexValue*>(hspmalloc( size ));

	std::memset( pt, 0x00, size );

	// 値を継承(複写 & src解放) or 初期化
	if ( pval2 ) {
		// 所有権ごと丸々コピー
		std::memmove(pt, pval2->pt, sizeof(FlexValue) * pval2->len[1]);

		// pval の方が短い場合、継承できない分を破棄
		auto const iter2 = StructTraits::asValptr(pval2->pt);
		for ( int i = pval2->len[1]; i < pval->len[1]; ++i ) {
			FlexValue_Release(iter2[i]);
		}

		hspfree(pval2->pt);
	}

	pval->mode   = HSPVAR_MODE_MALLOC;
	pval->pt     = StructTraits::asPDAT(pt);
	pval->size   = size;
//	pval->master = nullptr;	// 不使用
	return;
}

//------------------------------------------------
// 解放
//------------------------------------------------
void HspVarStructWrap_Free( PVal* pval )
{
	if ( pval->mode == HSPVAR_MODE_MALLOC ) {
		auto const fv = StructTraits::asValptr(pval->pt);
		for ( int i = 0; i < pval->len[1]; ++ i ) {
			FlexValue_Release( fv[i] );
		}
		hspfree( pval->pt );
	}
	else { assert(pval->mode == HSPVAR_MODE_CLONE || !pval->pt); }

	pval->mode = HSPVAR_MODE_NONE;
	pval->pt   = nullptr;
	pval->size = 0;
	return;
}

//------------------------------------------------
// 複写
//------------------------------------------------
void HspVarStructWrap_Set( PVal* pval, PDAT* pdat, PDAT const* in )
{
	auto& fv_dst = *StructTraits::asValptr(pdat);
	auto& fv_src = *StructTraits::asValptr(in);

	FlexValue_Copy( fv_dst, fv_src );

	// テンポラリ変数への代入(非破壊演算の左辺 | code_get() の返値)
	// テンポラリ変数を記憶しておく
	if ( !mpval_struct && (pval->support & HSPVAR_SUPPORT_TEMPVAR) ) {
		mpval_struct = pval;
	}

	FlexValue_ReleaseTmp(fv_src);
	return;
}

//------------------------------------------------
// 登録された演算実体を検索して取得する
// 
// @result: 関数子
//------------------------------------------------
static functor_t const& StructWrap_GetMethod( int subid, int opId )
{
	CModOperator const* const pModOp = getModOperator();

	// 演算関数リストを検索 
	auto const iterCls = pModOp->find( subid );
	if ( iterCls == pModOp->end() ) { dbgout("no operation defined in %d", subid); puterror( HSPERR_UNSUPPORTED_FUNCTION ); }

	// 実行処理の取得 
	auto const iterOp = iterCls->second.find( opId );
	if ( iterOp == iterCls->second.end() || iterOp->second->getUsing() == 0 ) { dbgout("operation #%d is not defined", opId); puterror( HSPERR_UNSUPPORTED_FUNCTION ); }

	functor_t const& functor = iterOp->second;

	return functor;
}

//------------------------------------------------
// 複製関数呼び出し
// 
// @result: 複製されたインスタンス (参照カウンタ: 1) or nullmod
//------------------------------------------------
void HspVarStructWrap_Dup( FlexValue* result, FlexValue* fv )
{
	if ( FlexValue_IsNull(*fv) ) {
		FlexValue_DelRef( *result );
		return;
	}

	// 一時変数 (これに複製を作ってもらう)
	PVal _pvTmp { };
	PVal* const pvTmp = &_pvTmp;

	PVal_init( pvTmp, HSPVAR_FLAG_STRUCT );

	// 呼び出し 
	{
		CCaller caller;
		functor_t const& functor = StructWrap_GetMethod( FlexValue_SubId(*fv), OpId_Dup );

		// 呼び出し準備 
		caller.setFunctor( functor );
		caller.addArgByVal( fv, HSPVAR_FLAG_STRUCT );
		caller.addArgByRef( pvTmp );

		// 呼び出し 
		caller.call();
	}

	if ( pvTmp->flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );
	FlexValue_Move( *result, *StructTraits::asValptr(pvTmp->pt) );

	PVal_free( pvTmp );
	return;
}

//------------------------------------------------
// ラップされた演算関数
// 
// @ 非破壊二項演算 ( += でなく、+ などのこと ) の場合、
// @	コード実行中に mpval ポインタが変わってしまうので、
// @	制御を戻す前に mpval を mpval_struct に直しておく。
// @ 非破壊なら、左辺のインスタンスを破壊してはいけないので、
// @	左辺の複製に対して処理を行う (一時オブジェクトを返す)。
// @ lhs(thismod) が null なら結果も null 、故に何もしない。
//------------------------------------------------
static void HspVarStructWrap_CoreI( PDAT* pdat, void const* val, int opId )
{
	// 非破壊二項演算の場合 (mpval_struct は既に取得できているはず)
	bool const bTempOp =
		(mpval_struct && (void*)mpval_struct->pt == pdat);

	auto const lhs = StructTraits::asValptr(pdat);
	auto const rhs = StructTraits::asValptr(val);
	assert(!!lhs && !!rhs);

	// 呼び出し
	if ( !FlexValue_IsNull(*lhs) ) {
		CCaller caller;

		functor_t const& functor = StructWrap_GetMethod( FlexValue_SubId(*lhs), opId );

		// 複製を生成する 
		if ( bTempOp ) {
			FlexValue _lhsDup = {0};
			FlexValue* const lhsDup = &_lhsDup;
			{
				FlexValueHolder lhsBak( *lhs );	// dup 中に壊れてしまいうるので、mpval_struct を保存
				HspVarStructWrap_Dup( lhsDup, lhs );
				FlexValue_AddRefTmp( *lhsDup );	// 返却されてスタックに乗るので
			}

			// lhs に lhsDup を代入する (元々あった方を lhsDup で保持)
			std::swap( *lhs, *lhsDup );
			
			// 左辺に元々あったインスタンスを破棄
			FlexValue_Release( *lhsDup );
		}

		// 呼び出し準備 
		caller.setFunctor( functor );
		caller.addArgByVal( lhs, HSPVAR_FLAG_STRUCT );
		caller.addArgByVal( rhs, HSPVAR_FLAG_STRUCT );

		// 呼び出し 
		caller.call();

	// 左辺が nullmod => 返値も nullmod
	} else {
		FlexValue_DelRef(*lhs);
	}

	getHvp(HSPVAR_FLAG_STRUCT)->aftertype = HSPVAR_FLAG_STRUCT;

	if ( bTempOp ) *(exinfo->mpval) = mpval_struct;	// restore

	// 右辺が一時オブジェクトなら、スタックから降りる
	FlexValue_ReleaseTmp( *rhs );
	return;
}

//------------------------------------------------
// ラップされた演算関数::比較
// 
// @ 複製を生成しない。
// @ CmpI を呼び、その返値を比較値として利用する。
// @ mpval_struct を破壊するのでなく、mpval を mpval_int に差し替えて、
// @	それに演算返値を代入する。
//------------------------------------------------
static void HspVarStructWrap_CmpI( PDAT* pdat, void const* val, int opId )
{
	// 非破壊二項演算の場合
	bool const bTempOp = (mpval_struct && (void*)mpval_struct->pt == pdat);	

	if ( !bTempOp ) {
		// 複合代入 => 左辺変数の型が変わるが、それはエラーになるので先にエラーを出しておく
		dbgout("int 型以外の複合代入比較演算は行えない。");
		puterror( HSPERR_TYPE_MISMATCH );
	}

	auto const lhs = StructTraits::asValptr(pdat);
	auto const rhs = StructTraits::asValptr(val);

	// 演算結果
	int cmp;

	// 呼び出し 
	if ( !FlexValue_IsNull(*lhs) && !FlexValue_IsNull(*rhs) ) {
		CCaller caller;
		functor_t const& functor = StructWrap_GetMethod( FlexValue_SubId(*lhs), OpId_Cmp );

		// 呼び出し準備 
		caller.setFunctor( functor );
		caller.addArgByVal(   lhs, HSPVAR_FLAG_STRUCT );
		caller.addArgByVal(   rhs, HSPVAR_FLAG_STRUCT );
		caller.addArgByVal( &opId, HSPVAR_FLAG_INT );

		// 呼び出し 
		caller.call();

		// 返値を取得 
		PDAT* result = nullptr;
		vartype_t const resVt = caller.getCallResult( &result );
		if ( resVt != HSPVAR_FLAG_INT ) puterror( HSPERR_TYPE_MISMATCH );

		cmp = VtTraits<int>::derefValptr(result);

	} else {
		// どちらかが nullmod のとき、nullmod でない方が大きいということにする
		cmp = (!FlexValue_IsNull(*lhs) ? 1 : 0) - (!FlexValue_IsNull(*rhs) ? 1 : 0);
	}

	// 演算返値を設定する
	int const calccode = opId &~ OpFlag_Calc;
	int const resultValue = HspBool(
		( calccode == CALCCODE_EQ   ) ? cmp == 0 :
		( calccode == CALCCODE_NE   ) ? cmp != 0 :
		( calccode == CALCCODE_GT   ) ? cmp >  0 :
		( calccode == CALCCODE_LT   ) ? cmp <  0 :
		( calccode == CALCCODE_GTEQ ) ? cmp >= 0 :
		( calccode == CALCCODE_LTEQ ) ? cmp <= 0 : (puterror( HSPERR_UNKNOWN_CODE ), false)
	);

	PVal* const pvRes = getMPVal(HSPVAR_FLAG_INT);
	if ( !pvRes ) { dbgout("mpval_int is unknown"); puterror( HSPERR_UNKNOWN_CODE ); }
	*(exinfo->mpval) = pvRes; 
	*(int*)pvRes->pt = resultValue;

	getHvp(HSPVAR_FLAG_STRUCT)->aftertype = HSPVAR_FLAG_INT;

	// 右辺がスタックから降りるので一時オブジェクトなら解放する
	FlexValue_ReleaseTmp( *rhs );
	return;
}

void HspVarStructWrap_AddI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_ADD ); }
void HspVarStructWrap_SubI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_SUB ); }
void HspVarStructWrap_MulI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_MUL ); }
void HspVarStructWrap_DivI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_DIV ); }
void HspVarStructWrap_ModI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_MOD ); }

void HspVarStructWrap_AndI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_AND ); }
void HspVarStructWrap_OrI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_OR  ); }
void HspVarStructWrap_XorI ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_XOR ); }

void HspVarStructWrap_EqI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_EQ   ); }
void HspVarStructWrap_NeI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_NE   ); }
void HspVarStructWrap_GtI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_GT   ); }
void HspVarStructWrap_LtI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_LT   ); }
void HspVarStructWrap_GtEqI( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_GTEQ ); }
void HspVarStructWrap_LtEqI( PDAT* pdat, void const* val ) { HspVarStructWrap_CmpI ( pdat, val, OpFlag_Calc | CALCCODE_LTEQ ); }

void HspVarStructWrap_RrI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_RR ); }
void HspVarStructWrap_LrI  ( PDAT* pdat, void const* val ) { HspVarStructWrap_CoreI( pdat, val, OpFlag_Calc | CALCCODE_LR ); }

//------------------------------------------------
// 型変換 (from)
//------------------------------------------------
PDAT* HspVarStructWrap_Cnv( PDAT const* buffer, int flag )
{
	// 指定型(flag) → struct
	if ( flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );
	return const_cast<PDAT*>(buffer);
}

//------------------------------------------------
// 型変換 (to)
// 
// @ CnvCustom: CnvTo ( struct → 何か )
//------------------------------------------------
PDAT* HspVarStructWrap_CnvCustom( PDAT const* buffer, int flag )
{
	int const opId = OpFlag_CnvTo | flag;
	auto const fv = StructTraits::asValptr(buffer);

	if ( flag == HSPVAR_FLAG_STRUCT ) return VtTraits<struct_tag>::asPDAT(const_cast<FlexValue*>(fv));

	PDAT* result = nullptr;

	// 呼び出し 
	if ( !FlexValue_IsNull(*fv) ) {
		PVal*& mpval = *exinfo->mpval;

		PVal _pvTmp = {0};			// mpval の値を保存
		PVal* const pvTmp = &_pvTmp;
		bool const bInExpr = (mpval->flag == flag);	// 演算式の右辺の型変換かも => コード実行で mpval を破壊してしまうので、保存する
		if ( bInExpr ) {
			PVal_init( pvTmp, HSPVAR_FLAG_INT );
			PVal_assign( pvTmp, mpval->pt, mpval->flag );
		}

		// 変換関数を呼び出す
		{
			CCaller caller;
			functor_t const& functor = StructWrap_GetMethod(FlexValue_SubId(*fv), opId);

			caller.setFunctor(functor);
			caller.addArgByVal(fv, HSPVAR_FLAG_STRUCT);

			caller.call();

			vartype_t resVt = caller.getCallResult(&result);
			if ( resVt != flag ) puterror(HSPERR_TYPE_MISMATCH);
		}

		// 後始末 
		if ( bInExpr ) {
			mpval = getMPVal(flag);
			PVal_assign( mpval, pvTmp->pt, pvTmp->flag );	// mpval の値を restore
			PVal_free( pvTmp );
		}
		
		// スタックから降りる
		FlexValue_ReleaseTmp(*fv);

	} else {	// nullmod → 他
		static PVal* pval;
		if ( !pval ) {
			pval = reinterpret_cast<PVal*>(hspmalloc( sizeof(PVal) ));
			PVal_init( pval, flag );
		}
		switch ( flag ) {
			case HSPVAR_FLAG_LABEL:
			{
				static label_t const lb = nullptr; PVal_assign( pval, VtTraits<label_t>::asPDAT(&lb), flag ); break;
			}
			case HSPVAR_FLAG_STR:
			{
				static char const* const s = ""; PVal_assign( pval, VtTraits<str_tag>::asPDAT(s), flag ); break;
			}
			case HSPVAR_FLAG_DOUBLE:
			{
				static double const r = std::numeric_limits<double>::quiet_NaN();
				PVal_assign( pval, VtTraits<double>::asPDAT(&r), flag ); break;
			}
			case HSPVAR_FLAG_INT:
			{
				static int const n = 0; PVal_assign( pval, VtTraits<int>::asPDAT(&n), flag ); break;
			}
			default: puterror( HSPERR_UNSUPPORTED_FUNCTION );
		}
		result = pval->pt;
	}

	return result;
}

//------------------------------------------------
// 型変換関数のラップ
// 
// @ 組み込み型の Cnv を、struct のときに特殊処理するようにラップする。
// @	組み込み型は「int 型への変換」の関数を用いるので、
// @	それに struct が与えられたときに HspvarStructWrap_CnvCustom を用いるようにする。
// @ 元々の関数は g_cnvfunc_impl に保存する。
//------------------------------------------------
/*
#include <functional>

typedef void* (*cnvfunc_t)( void const* buffer, int flag );
typedef std::function<void*( void const*, int )> cnvfuncLambda_t;

static std::array<cnvfunc_t,       HSPVAR_FLAG_USERDEF> g_cnvfunc_impl;	// 元々設定されていた Cnv 関数
static std::array<cnvfuncLambda_t, HSPVAR_FLAG_USERDEF> g_cnvfunc_wrap;	// ラップした Cnv 関数

void HspVarStructWrap_InitCnvWrap()
{
	// ラッパー関数を生成する関数
	auto makeCnvFuncWrapper = []( vartype_t this_type ) {
		return [this_type]( void const* buffer, int flag ) {
			return ( flag == HSPVAR_FLAG_STRUCT )
				? HspVarStructWrap_CnvCustom( buffer, flag )
				: g_cnvfunc_impl[ this_type ]( buffer, flag );
		};
	};

	// 各組み込み型の Cnv をラップする
	for ( int i = 1; i <= HSPVAR_FLAG_STRUCT; ++ i ) {
		HspVarProc* vp = getHvp(i);

		g_cnvfunc_wrap[i] = makeCnvFuncWrapper( i );
		g_cnvfunc_impl[i] = vp->Cnv;
		vp->Cnv = g_cnvfunc_wrap[i];		// function<> は関数ポインタにできない
	}

	return;
}

/*/
typedef PDAT* (*cnvfunc_t)( PDAT const* buffer, int flag );
static std::array<cnvfunc_t, HSPVAR_FLAG_USERDEF> g_cnvfunc_impl;	// 元々設定されていた Cnv 関数
static std::array<cnvfunc_t, HSPVAR_FLAG_USERDEF> g_cnvfunc_wrap;	// ラップする Cnv 関数

#if 0
#define FTM_HspVarStructWrap_CnvWrap(Name, Type) \
	static void* HspVarStructWrap_CnvWrap_##Name( const void *buffer, int flag )	\
	{\
		return ( flag == HSPVAR_FLAG_STRUCT )				\
			? HspVarStructWrap_CnvCustom( buffer, Type )	\
			: g_cnvfunc_impl[ Type ]( buffer, flag );		\
	}

FTM_HspVarStructWrap_CnvWrap( Label,  HSPVAR_FLAG_LABEL  );
FTM_HspVarStructWrap_CnvWrap( Str,    HSPVAR_FLAG_STR    );
FTM_HspVarStructWrap_CnvWrap( Double, HSPVAR_FLAG_DOUBLE );
FTM_HspVarStructWrap_CnvWrap( Int,    HSPVAR_FLAG_INT    );
FTM_HspVarStructWrap_CnvWrap( Struct, HSPVAR_FLAG_STRUCT );
#endif

template<vartype_t Type>
PDAT* HspVarStructWrap_CnvWrap( PDAT const* buffer, int flag )
{
	return ( flag == HSPVAR_FLAG_STRUCT )
		? HspVarStructWrap_CnvCustom( buffer, Type )
		: g_cnvfunc_impl[ Type ]( buffer, flag );
}
//template<> void* HspVarStructWrap_CnvWrap<HSPVAR_FLAG_STRUCT>( void const* buffer, int flag )
//{ return HspVarStructWrap_CnvCustom( buffer, HSPVAR_FLAG_STRUCT ); }

void HspVarStructWrap_InitCnvWrap()
{
	g_cnvfunc_wrap[ HSPVAR_FLAG_LABEL  ] = HspVarStructWrap_CnvWrap<HSPVAR_FLAG_LABEL>;
	g_cnvfunc_wrap[ HSPVAR_FLAG_STR    ] = HspVarStructWrap_CnvWrap<HSPVAR_FLAG_STR>;
	g_cnvfunc_wrap[ HSPVAR_FLAG_DOUBLE ] = HspVarStructWrap_CnvWrap<HSPVAR_FLAG_DOUBLE>;
	g_cnvfunc_wrap[ HSPVAR_FLAG_INT    ] = HspVarStructWrap_CnvWrap<HSPVAR_FLAG_INT>;

	// 各組み込み型の Cnv をラップする
	for ( int i = 1; i < HSPVAR_FLAG_STRUCT; ++ i ) {
		auto const vp = getHvp(i);

		g_cnvfunc_impl[i] = vp->Cnv;
		vp->Cnv = g_cnvfunc_wrap[i];
	}

	return;
}
//*/

//------------------------------------------------
// メソッド
// 
// @ OpId_Method に登録された命令を呼び、
// @	そこで返却された functor を呼び出す。
//------------------------------------------------
void HspVarStructWrap_Method( PVal* pval )
{
	FlexValue& self = *reinterpret_cast<FlexValue*>( getHvp(HSPVAR_FLAG_STRUCT)->GetPtr(pval) );
	FlexValue_AddRef( self );

	char* const method = code_gets();

	if ( FlexValue_IsNull(self) ) return;

	functor_t functorImpl;		// 実際のメソッド
	int const exflg_bak = *exinfo->npexflg;	// 保存

	// 分散関数の呼び出し 
	{
		CCaller caller;
		functor_t const& functorMethod = StructWrap_GetMethod( FlexValue_SubId(self), OpId_Method );

		caller.setFunctor( functorMethod );
		caller.addArgByVal( &self, HSPVAR_FLAG_STRUCT );
		caller.addArgByVal( method, HSPVAR_FLAG_STR );

		caller.call();

		PDAT* result;
		vartype_t const resultType = caller.getCallResult( &result );

		functorImpl = *reinterpret_cast<functor_t*>( g_hvpFunctor->Cnv( result, resultType ) );
	}

	*exinfo->npexflg = exflg_bak;	// restore

	// メソッド実体の呼び出し 
	{
		CCaller caller;

		// 呼び出し準備 
		caller.setFunctor( functorImpl );

		int const prmtype = functorImpl->getPrmInfo().getPrmType(0);
		if ( prmtype == PRM_TYPE_MODVAR || prmtype == PRM_TYPE_VAR
		  || prmtype == HSPVAR_FLAG_STRUCT || prmtype == PRM_TYPE_ANY
		 ) {		// 第一引数に modvar を渡せる場合 thismod を渡す
			caller.addArgByVal( &self, HSPVAR_FLAG_STRUCT );
		}
		
		// スクリプトから引数を取り出す
		caller.setArgAll();

		// 呼び出し 
		caller.call();
	}

	FlexValue_Release( self );
	return;
}
