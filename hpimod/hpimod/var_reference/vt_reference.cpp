// reference - VarProc code

#include "for_knowbug.var_reference.h"
#include "vt_reference.h"

#include "hsp3plugin_custom.h"
#include "mod_makepval.h"
#include "mod_argGetter.h"

// 変数の定義
short g_vtReference;
HspVarProc* g_pHvpReference;

// 関数の宣言
extern PDAT* HspVarReference_GetPtr         ( PVal* pval) ;
extern int   HspVarReference_GetSize        ( const PDAT* pdat );
extern int   HspVarReference_GetUsing       ( const PDAT* pdat );
extern void* HspVarReference_GetBlockSize   ( PVal* pval, PDAT* pdat, int* size );
extern void  HspVarReference_AllocBlock     ( PVal* pval, PDAT* pdat, int  size );
extern void  HspVarReference_Alloc          ( PVal* pval, const PVal* pval2 );
extern void  HspVarReference_Free           ( PVal* pval);
extern void* HspVarReference_ArrayObjectRead( PVal* pval, int* mptype );
extern void  HspVarReference_ArrayObject    ( PVal* pval);
extern void  HspVarReference_ObjectWrite    ( PVal* pval, void* data, int vflag );
extern void  HspVarReference_ObjectMethod   ( PVal* pval);

//------------------------------------------------
// アクセス用
//------------------------------------------------
namespace VtReference {
	valptr_t GetPtr( PVal* pval )
	{
		assert( pval     != nullptr );
		assert( pval->pt != nullptr );
		return (valptr_t)pval->pt;
	}
	
	static PVal* GetPVal( const value_t& v ) {
		return reinterpret_cast<PVal*>( v.master );
	}
	
	static APTR GetAPTR( const value_t& v ) {
		return v.arraymul;
	}
	
	static PVal* GetPVal( PVal* pval ) { return GetPVal( *GetPtr(pval) ); }
	static APTR  GetAPTR( PVal* pval ) { return GetAPTR( *GetPtr(pval) ); }
}

//------------------------------------------------
// Core
//------------------------------------------------
static PDAT* HspVarReference_GetPtr( PVal* pval )
{
	return (PDAT*)( VtReference::GetPtr(pval) );
}

//------------------------------------------------
// Size
//------------------------------------------------
static int HspVarReference_GetSize( const PDAT* pdat )
{
	return VtReference::basesize;
}

//------------------------------------------------
// Using
//------------------------------------------------
static int HspVarReference_GetUsing( const PDAT* pdat )
{
	return VtReference::GetPVal(*reinterpret_cast<const VtReference::value_t*>(pdat)) != nullptr ? 1 : 0;
}

//------------------------------------------------
// ブロックメモリ
//------------------------------------------------
static void* HspVarReference_GetBlockSize( PVal* pval, PDAT* pdat, int* size )
{
	*size = pval->size - ( ((char*)pdat) - ((char*)pval->pt) );
	return pdat;
}

static void HspVarReference_AllocBlock( PVal* pval, PDAT* pdat, int size )
{
	return;
}

//------------------------------------------------
// PValの変数メモリを確保する
//
// @ pval は未確保 or 解放済みの状態。
// @ pval2 != NULL => pval2の内容を継承する。
//------------------------------------------------
static void HspVarReference_Alloc( PVal* pval, const PVal* pval2 )
{
	// 配列として参照できるが、配列にはできない
	pval->len[1] = 1;
	pval->len[2] = 0;
	
	// 継承
	if ( pval2 != NULL ) {
		pval->master   = pval2->master;
		pval->arraymul = pval2->arraymul;
		
	} else {
		pval->master   = nullptr;
		pval->arraymul = 0;		// aptr として使う
	}
	
	// pval へ設定
	pval->flag = g_vtReference;		// reference の型タイプ値
	pval->mode = HSPVAR_MODE_MALLOC;
	pval->size = VtReference::basesize;
	pval->pt   = (char*)&pval->master;		// 必ず自身の master への参照となっている
	return;
}

//------------------------------------------------
// PValの変数メモリを解放する
//------------------------------------------------
static void HspVarReference_Free( PVal* pval )
{
	if ( pval->mode == HSPVAR_MODE_MALLOC ) {
		// リソースを使わない
	}
	
	pval->pt     = nullptr;
	pval->mode   = HSPVAR_MODE_NONE;
	pval->master = nullptr;
	return;
}

//------------------------------------------------
// 代入 (=)
// 
// @ 参照共有
//------------------------------------------------
static void HspVarReference_Set( PVal* pval, PDAT* pdat, const void* in )
{
	auto rhs = reinterpret_cast<const VtReference::value_t*>(in);
	
	pval->master   = rhs->master;
	pval->arraymul = rhs->arraymul;
	return;
}

//------------------------------------------------
// Reference 登録関数
//------------------------------------------------
void HspVarReference_Init( HspVarProc* p )
{
	g_pHvpReference = p;
	g_vtReference   = p->flag;
	
	// 関数ポインタを登録
	p->GetPtr       = HspVarReference_GetPtr;
	p->GetSize      = HspVarReference_GetSize;
	p->GetUsing     = HspVarReference_GetUsing;
	
	p->Alloc        = HspVarReference_Alloc;
	p->Free         = HspVarReference_Free;
	p->GetBlockSize = HspVarReference_GetBlockSize;
	p->AllocBlock   = HspVarReference_AllocBlock;
	
	// 演算関数
	p->Set          = HspVarReference_Set;
//	p->AddI         = HspVarReference_AddI;
//	p->EqI          = HspVarReference_EqI;
//	p->NeI          = HspVarReference_NeI;
	
	// 連想配列用
	p->ArrayObjectRead = HspVarReference_ArrayObjectRead;	// 参照(右)
	p->ArrayObject     = HspVarReference_ArrayObject;		// 参照(左)
	p->ObjectWrite     = HspVarReference_ObjectWrite;		// 格納
//	p->ObjectMethod    = HspVarReference_ObjectMethod;		// メソッド
	
	// 拡張データ
//	p->user         = (char*)HspVarReference_GetMapList;
	
	// その他設定
	p->vartype_name = "reference_k";	// タイプ名 (衝突しないように変な名前にする)
	p->version      = 0x001;			// runtime ver(0x100 = 1.0)
	
	p->support							// サポート状況フラグ(HSPVAR_SUPPORT_*)
		= HSPVAR_SUPPORT_STORAGE		// 固定長ストレージ
	//	| HSPVAR_SUPPORT_FLEXARRAY		// 可変長配列
		| HSPVAR_SUPPORT_ARRAYOBJ		// 連想配列サポート
		| HSPVAR_SUPPORT_NOCONVERT		// ObjectWriteで格納
		| HSPVAR_SUPPORT_VARUSE			// varuse関数を適用
		;
	p->basesize = VtReference::basesize;	// size / 要素 (byte)
	return;
}

//#########################################################
//        連想配列用の関数群
//#########################################################
//------------------------------------------------
// 連想配列::参照 (左辺値)
//------------------------------------------------
static void HspVarReference_ArrayObject( PVal* pval )
{
	if ( !pval->master ) puterror( HSPERR_VARIABLE_REQUIRED );
	
	PVal* const pvInner = VtReference::GetPVal( pval );
	APTR  const apRef   = VtReference::GetAPTR( pval );
	
	// ダミー添字を捨てる
	if ( code_geti() != VtReference::IdxDummy ) puterror( HSPERR_ARRAY_OVERFLOW );
	
	// 内部変数の添字を処理
	if ( code_isNextArg() ) {				// 添字がある場合
		if ( apRef > 0 ) puterror( HSPERR_INVALID_ARRAY );	// 配列要素参照 => 添字過剰
		code_expand_index_impl_lhs( pvInner );
		
	} else {
		if ( PVal_supportArray(pvInner) && !(pval->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {		// 標準配列サポート
			HspVarCoreReset( pvInner );								// 添字状態の初期化
			if ( apRef > 0 ) code_index_int_rhs( pvInner, apRef );	// 要素添字
		}
	}
	
	/*
	// pval を内部変数のクローンにする
	PVal_cloneVar( pval, pvInner,
		(apRef > 0)
			? apRef
			: ( pvInner->arraycnt > 0 ? pvInner->offset : -1 )
	);
	pvInner->support |= HSPVAR_SUPPORT_ARRAYOBJ;
	//*/
	
	return;
}

//------------------------------------------------
// 連想配列::参照 (右辺値)
//------------------------------------------------
static void* HspVarReference_ArrayObjectRead( PVal* pval, int* mptype )
{
	// ダミー添字を捨てる
	if ( code_geti() != VtReference::IdxDummy ) dbgout("!need dummy idx");//puterror( HSPERR_ARRAY_OVERFLOW );
	
	if ( !pval->master ) puterror( HSPERR_VARIABLE_REQUIRED );
	
	PVal* const pvInner = VtReference::GetPVal( pval );
	APTR  const apRef   = VtReference::GetAPTR( pval );
	
	// 内部変数の添字を処理
	if ( code_isNextArg() ) {				// 添字がある場合
		if ( apRef > 0 ) puterror( HSPERR_INVALID_ARRAY );	// 配列要素参照 => 添字過剰
		return code_expand_index_impl_rhs( pvInner, mptype );
		
	} else {
		if ( PVal_supportArray(pvInner) && !(pval->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {		// 標準配列サポート
			HspVarCoreReset( pvInner );								// 添字状態の初期化
			if ( apRef > 0 ) code_index_int_rhs( pvInner, apRef );	// 要素添字
		}
		
		*mptype = pvInner->flag;
		return GetHvp( pvInner->flag )->GetPtr( pvInner );
	}
}

//------------------------------------------------
// 連想配列::格納
//------------------------------------------------
static void HspVarReference_ObjectWrite( PVal* pval, void* data, int vflag )
{
	PVal* const pvInner = VtReference::GetPVal( *VtReference::GetPtr(pval) );
	
	// reference への代入
	if ( !pvInner ) {
		if ( vflag != g_vtReference ) puterror( HSPERR_INVALID_ARRAYSTORE );	// 右辺の型が不一致
		
		HspVarReference_Set( pval, (PDAT*)HspVarReference_GetPtr(pval), data );
		
	// 内部変数を参照している場合
	} else {
		PVal_assign( pvInner, data, vflag );	// 内部変数への代入処理
		code_assign_multi( pvInner );
	}
	
	return;
}

//------------------------------------------------
// メソッド呼び出し
// 
// @ 内部変数の型で提供されているメソッドを使う
//------------------------------------------------
static void HspVarReference_ObjectMethod(PVal* pval)
{
	PVal* const pvInner = VtReference::GetPVal( pval );
	if ( !pvInner ) puterror( HSPERR_UNSUPPORTED_FUNCTION );
	
	// 内部変数の処理に転送
	GetHvp(pvInner->flag)->ObjectMethod( pvInner );
	
	return;
}
