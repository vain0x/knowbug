// Call(ModCls) - Command
#if 0
#include <array>
#include <map>
#include <string>

#include "mod_func_result.h"

#include "cmd_modcls.h"
#include "cmd_sub.h"

#include "CCaller.h"
#include "CMethod.h"
#include "CMethodlist.h"
#include "CCall.h"
#include "CPrmInfo.h"
#include "vt_functor.h"

#include "modcls_FlexValue.h"
#include "vt_structWrap.h"
#include "CModClsCtor.h"

using namespace hpimod;
using namespace ModCls;

//################################################
//    グローバル変数
//################################################

// null オブジェクトを有する変数
static PVal* g_nullmod;
const  PVal* ModCls::getNullmod() { return g_nullmod; }

// 演算関数の登録リスト
static CModOperator* g_pModOp;
const  CModOperator* ModCls::getModOperator() { return g_pModOp; }

// 固有オブジェクトのリスト
static std::map<stdat_t, functor_t>* g_pModClsIdentity;
functor_t const& getModClsCtor( stdat_t modcls );

// 返値返却用
static FlexValue stt_resfv;

//------------------------------------------------
// struct 型の値を取り出す
//------------------------------------------------
static FlexValue* code_get_fv()
{
	int const chk = code_getprm();
	if ( chk <= PARAM_END ) puterror(HSPERR_NO_DEFAULT);
	if ( mpval->flag != HSPVAR_FLAG_STRUCT ) puterror(HSPERR_TYPE_MISMATCH);
	return reinterpret_cast<FlexValue*>(mpval->pt);
}

//------------------------------------------------
// struct 型の値を返却する
//------------------------------------------------
template<>
int hpimod::SetReffuncResult<FlexValue>(void** ppResult, FlexValue const& fv, int vtype)
{
	assert(vtype == HSPVAR_FLAG_STRUCT);

	FlexValue_Copy(stt_resfv, fv);
	*ppResult = &stt_resfv;

	FlexValue_AddRefTmp(stt_resfv);		// スタックに積まれるので
	return HSPVAR_FLAG_STRUCT;
}

static int SetReffuncResult(void** ppResult, FlexValue const& fv)
{ return SetReffuncResult(ppResult, fv, HSPVAR_FLAG_STRUCT); }

// move semantics
static int SetReffuncResult(void** ppResult, FlexValue&& fv)
{
	FlexValue_Move(stt_resfv, fv);
	*ppResult = &stt_resfv;

	FlexValue_AddRefTmp(stt_resfv);		// スタックに積まれるので
	return HSPVAR_FLAG_STRUCT;
}

//------------------------------------------------
// モジュールクラス機能が有効かどうか
//------------------------------------------------
bool ModCls_IsWorking()
{
	return (g_nullmod != nullptr);
}

//------------------------------------------------
// モジュールクラス機能の開始
// 
// @ struct 型を乗っ取る。
//------------------------------------------------
void ModCls_Init()
{
	if ( ModCls_IsWorking() ) return;		// 初期化済み

	HspVarStructWrap_Init( getHvp(HSPVAR_FLAG_STRUCT) );

	g_nullmod = new PVal;
	PVal_init( g_nullmod, HSPVAR_FLAG_STRUCT );

	g_pModOp          = new CModOperator;
	g_pModClsIdentity = new std::remove_pointer_t<decltype(g_pModClsIdentity)>;
	return;
}

//------------------------------------------------
// 終了時
//------------------------------------------------
void ModCls_Term()
{
	if ( !ModCls_IsWorking() ) return;		// 未初期化 or 解放済み

	// テンポラリ変数が持つ参照を除去する
	if ( getMPValStruct() ) {
		FlexValue_DelRef( *StructTraits::asValptr(getMPValStruct()->pt) );
	}

	// 全ての静的変数が持つ参照を除去する
	for ( int i = 0; i < ctx->hsphed->max_val; ++ i ) {
		PVal* const it = &ctx->mem_var[i];

		if ( it->flag == HSPVAR_FLAG_STRUCT ) {
			for ( int k = 0; k < it->len[1]; ++ k ) {
				FlexValue_DelRef( StructTraits::asValptr(it->pt)[k] );
			}
		}
	}

	FlexValue_DelRef( stt_resfv );
	PVal_free( g_nullmod );

	delete g_nullmod; g_nullmod = nullptr;
	delete g_pModOp;  g_pModOp  = nullptr;
	delete g_pModClsIdentity; g_pModClsIdentity = nullptr;
	return;
}

//------------------------------------------------
// 演算関数を登録する
// 
// @prm p1 = modcls  : モジュールクラス
// @prm p2 = OpId    : OpId_*
// @prm p3 = functor : 処理
//------------------------------------------------
void ModCls_Register()
{
	// 実引数
	stdat_t const pStDat  = code_get_modcls();
	int const     opId    = code_geti();
	functor_t&&    functor = code_get_functor();

	// モジュールクラスを検索
	auto iter = g_pModOp->find( pStDat->subid );
	if ( iter == g_pModOp->end() ) {		// 未登録 => 登録する
		iter = g_pModOp->insert({ pStDat->subid, OpFuncDefs() }).first;
	}

	OpFuncDefs& defs = iter->second;

	// 演算関数を登録
	defs[opId] = functor;

	return;
}

//------------------------------------------------
// newmod (override)
// 
// @ 新しいインスタンスを生成する。
//------------------------------------------------
static APTR       code_newstruct( PVal *pval );
static FlexValue* code_reserve_modinst( PVal* pval, APTR aptr );

// 命令形式
void ModCls_Newmod()
{
	PVal* const pval = code_get_var();
	stdat_t const modcls = code_get_modcls();

	// 配列の新規要素を生成する
	APTR const aptr = code_newstruct( pval );
	FlexValue* const fv = code_reserve_modinst( pval, aptr );

	// コンストラクタ実行 (残りの引数はここで処理される)
	FlexValue_Ctor( *fv, modcls, pval, aptr );

	// 参照カウンタ 1 で生成されるが、これは変数によって所有される分とみなす。
	dbgout("<%08X> new: %d", fv, FlexValue_Counter(*fv));
	return;
}

// 関数形式
int ModCls_Newmod( void** ppResult, bool bSysvarForm )
{
	FlexValue self { };

	stdat_t const kls = code_get_modcls();

	if ( bSysvarForm ) {
		// [ modnew Modcls ( ... ) - ]
		if ( *type == TYPE_MARK && *val == '(' ) {
			code_next();
			FlexValue_Ctor( self, kls );			// 残りの引数はここで処理される
			if ( !code_next_expect( TYPE_MARK, ')') ) puterror( HSPERR_INVALID_FUNCPARAM );

		// [ modnew Modcls - ]
		// 引数なしコンストラクタを呼ぶ
		} else {
			*exinfo->npexflg |= EXFLG_1;		// 次の引数はないとして処理させる
			FlexValue_Ctor( self, kls );
			*exinfo->npexflg &= ~EXFLG_1;
		}
		if ( !code_next_expect( TYPE_MARK, CALCCODE_SUB ) ) puterror( HSPERR_SYNTAX );

	} else {
		// newmod( Modcls, ... )
		FlexValue_Ctor( self, kls );			// 残りの引数はここで処理される
	}

	return SetReffuncResult(ppResult, std::move(self));
}

// 次に要素を生成すべき添字を取得する
APTR code_newstruct( PVal* pval )
{
	if ( pval->flag != HSPVAR_FLAG_STRUCT ) return 0;	// ([0] への別の型の代入 → 型変換される)

	size_t const last = pval->len[1];
	auto const fv = StructTraits::asValptr(mpval->pt);

	for ( size_t i = 0; i < last; ++i ) {
		if ( fv[i].type == FLEXVAL_TYPE_NONE ) return i;
	}

	exinfo->HspFunc_redim(pval, 1, last + 1);		// 配列を拡張する
	return last;
}

// struct 型の変数のデータ領域を確保する
// @ 未確保状態にする
FlexValue* code_reserve_modinst( PVal* pval, APTR aptr )
{
	// 配列を自動拡張させる
	code_setva( pval, aptr, HSPVAR_FLAG_STRUCT, g_nullmod->pt );

	auto const fv = StructTraits::asValptr(PVal_getptr( pval, aptr ));
	assert(fv->type == FLEXVAL_TYPE_NONE && !fv->ptr);

	return fv;
}

//------------------------------------------------
// delmod (override)
// 
// @ 変数要素を nullmod にする。
// @ さらに、mpval などが所有している場合も破棄して、できるだけ dtor が実行されるようにする。
//------------------------------------------------
void ModCls_Delmod()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );

	auto const fv = StructTraits::asValptr(PVal_getptr(pval));

	if ( fv->ptr ) {
		if ( getMPValStruct() ) {
			auto const mpval_fv = StructTraits::asValptr(getMPValStruct()->pt);
			if ( mpval_fv && fv->ptr == mpval_fv->ptr ) FlexValue_DelRef( *mpval_fv );
		}

		if ( fv->ptr == stt_resfv.ptr ) FlexValue_DelRef( stt_resfv );
	}

	FlexValue_DelRef( *fv );
	return;
}

//------------------------------------------------
// nullmod
// 
// @ 空の FlexValue を返却する
//------------------------------------------------
int ModCls_Nullmod( void** ppResult )
{
	*ppResult = g_nullmod->pt;
	return HSPVAR_FLAG_STRUCT;
}

//------------------------------------------------
// dupmod
// 
// @ 複製 Factory
//------------------------------------------------
// 命令形式
void ModCls_Dupmod()
{
	PVal* const pvSrc = code_get_var();
	PVal* const pvDst = code_get_var();		// 複製先

	if ( pvSrc->flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );

	auto const src = StructTraits::asValptr(PVal_getptr(pvSrc));

	FlexValue* const dst = code_reserve_modinst( pvDst, pvDst->offset );
	HspVarStructWrap_Dup( dst, src );
	return;
}

// 関数形式
int ModCls_Dupmod( void** ppResult )
{
	FlexValue* const pSrc = code_get_fv();
	if ( !pSrc ) puterror( HSPERR_ILLEGAL_FUNCTION );

	FlexValue self { };
	{
		FlexValue src { };
		FlexValue_Copy( src, *pSrc );
		FlexValue_AddRef( src );
		{
			HspVarStructWrap_Dup( &self, &src );
		}
		FlexValue_Release( src );
	}
	return SetReffuncResult(ppResult, std::move(self));
}

//------------------------------------------------
// モジュールクラスの名前
//------------------------------------------------
int ModCls_Name( void** ppResult )
{
	stdat_t const modcls  = code_get_modcls();

	return SetReffuncResult( ppResult, ModCls_Name(modcls) );
}

//------------------------------------------------
// 固有オブジェクトを得る
//------------------------------------------------
int ModCls_Identity( void** ppResult )
{
	stdat_t const modcls = code_get_modcls();

	return SetReffuncResult( ppResult, getModClsCtor(modcls) );
}

functor_t const& getModClsCtor( stdat_t modcls )
{
	auto const iter = g_pModClsIdentity->find( modcls );
	if ( iter != g_pModClsIdentity->end() ) {	// キャッシュ済み
		return iter->second;

	} else {
		functor_t functor { static_cast<exfunctor_t>(CModClsCtor::New(modcls)) };
		return g_pModClsIdentity->insert({ modcls, functor }).first->second;
	}
}

//------------------------------------------------
// クラスを得る
//------------------------------------------------
int ModInst_Cls( void** ppResult )
{
	FlexValue* const src = code_get_modinst();
	auto       const kls = FlexValue_ModCls(*src);
	return SetReffuncResult( ppResult, getModClsCtor(kls) );
}

//------------------------------------------------
// クラス名を得る
//------------------------------------------------
int ModInst_ClsName( void** ppResult )
{
	FlexValue* const src = code_get_modinst();
	return SetReffuncResult( ppResult, FlexValue_ClsName(*src) );
}

//------------------------------------------------
// 同一性比較
//------------------------------------------------
int ModInst_Identify( void** ppResult )
{
	void* member_lhs = code_get_modinst()->ptr;
	void* member_rhs = code_get_modinst()->ptr;
	return SetReffuncResult( ppResult, HspBool(member_lhs == member_rhs) );
}

//------------------------------------------------
// thismod
// 
// @ thismod@hsp も使用できる。
// @ このコマンドは、値として使える。
//------------------------------------------------
int ModCls_This( void** ppResult )
{
	auto const thismod = reinterpret_cast<MPModVarData*>(ctx->prmstack);
	if ( thismod->magic != MODVAR_MAGICCODE ) puterror( HSPERR_INVALID_STRUCT_SOURCE );
	if ( thismod->pval->flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );

	*ppResult = PVal_getPtr(thismod->pval, thismod->aptr);
	return HSPVAR_FLAG_STRUCT;
}

//------------------------------------------------
// 
//------------------------------------------------
#endif
