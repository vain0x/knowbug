// vartype - functor

#include "vt_functor.h"
#include "cmd_call.h"
#include "cmd_sub.h"
#include "iface_call.h"

#include "CCaller.h"
#include "CBound.h"
#include "axcmd.h"

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"

#include <new>

using namespace hpimod;

vartype_t g_vtFunctor;
HspVarProc* g_hvpFunctor;
functor_t g_resFunctor { nullptr };

static void Call_functorVar(PVal* pval, CCaller& caller);
static void Call_functorVar(functor_t const& functor, CCaller& caller);

//------------------------------------------------
// 使用状況(varuse)
//------------------------------------------------
static int HspVarFunctor_getUsing( PDAT const* pdat )
{
	functor_t const& functor = FunctorTraits::derefValptr(pdat);
	return functor->getUsing();
}

//------------------------------------------------
// PValの変数メモリを確保する
// 
// @ pval は未確保 or 解放済み。
// @ pval2 != NULL => pval2 を継承。
// @ 配列は一次元のみ。
//------------------------------------------------
static void HspVarFunctor_alloc(PVal* pval, PVal const* pval2)
{
	if ( pval->len[1] < 1 ) pval->len[1] = 1;		// 配列を最低 1 は確保する
	if ( pval->len[2] != 0 ) puterror(HSPERR_ARRAY_OVERFLOW);

	size_t const cntElems = pval->len[1];
	size_t const size     = cntElems * FunctorTraits::basesize;
	
	functor_t* const pt = (functor_t*)hspmalloc( size );
	size_t offset = 0;

	// 継承
	if ( pval2 ) {
		offset = ( static_cast<size_t>(pval2->size) / sizeof(functor_t) );
		std::memcpy( pt, pval2->pt, pval2->size );		// 持っていたデータをコピー
		hspfree( pval2->pt );							// 元のバッファを解放
	}

	// pt の初期化 (null 初期化)
	for ( size_t i = offset; i < cntElems; ++ i ) {
		functor_t const* const p = new( &pt[i] ) functor_t;
	}

	pval->flag   = g_vtFunctor;
	pval->mode   = HSPVAR_MODE_MALLOC;
	pval->size   = size;
	pval->pt     = FunctorTraits::asPDAT(pt);
//	pval->master = nullptr;
	return;
}

//------------------------------------------------
// PValの変数メモリを解放する
//------------------------------------------------
static void HspVarFunctor_free(PVal* pval)
{
	if ( pval->mode == HSPVAR_MODE_MALLOC ) {
		// デストラクタ起動
		auto const pt = FunctorTraits::asValptr(pval->pt);
		for ( int i = 0; i < pval->len[1]; ++ i ) {
			pt[i].~Managed();
		}
		hspfree( pval->pt );
	}

	pval->mode = HSPVAR_MODE_NONE;
	pval->pt   = nullptr;
	return;
}

//------------------------------------------------
// 型変換処理
// 
// @ 他 -> functor
// @ g_resFunctor は SetReffuncResult と共用。
//------------------------------------------------
static void* HspVarFunctor_cnv(PDAT const* pdat, int flag)
{
	static functor_t& stt_cnv = g_resFunctor;

	switch ( flag ) {
		case HSPVAR_FLAG_LABEL:
			stt_cnv = Functor_New(VtTraits<label_t>::derefValptr(pdat));
			break;

		case HSPVAR_FLAG_INT:
		{
			stt_cnv = Functor_New(VtTraits<int>::derefValptr(pdat));
			break;
		}
		default:
			if ( flag == g_vtFunctor ) {
				stt_cnv = FunctorTraits::derefValptr(pdat);

			} else {
				puterror( HSPERR_TYPE_MISMATCH );
			}
			break;
	}

	return &stt_cnv;
}

//------------------------------------------------
// 型変換処理
// 
// @ functor -> 他
//------------------------------------------------
static void* HspVarFunctor_cnvCustom(PDAT const* pdat, int flag)
{
	auto const& functor = FunctorTraits::derefValptr(pdat);
	void* pResult = nullptr;

	switch ( flag ) {
		case HSPVAR_FLAG_LABEL:
		{
			static label_t stt_label;

			stt_label = functor->getLabel();
			if ( stt_label ) pResult = &stt_label;
			break;
		}
		case HSPVAR_FLAG_INT:
		{
			static int stt_int;

			stt_int = functor->getAxCmd();
			if ( AxCmd::isOk(stt_int) ) pResult = &stt_int;
			break;
		}

		default:
			if ( flag == g_vtFunctor ) {
				pResult = const_cast<PDAT*>(pdat);
			}
			break;
	}

	functor.beNonTmpObj();	// スタックから降りる

	if ( pResult ) {
		return pResult;
	} else {
		puterror(HSPERR_TYPE_MISMATCH); throw;
	}
}

//------------------------------------------------
// 連想配列 : 参照 (右)
// 
// @ 単体 => 関数形式呼び出し
// @ 配列 => 要素取得
//------------------------------------------------
static void* HspVarFunctor_arrayObjectRead( PVal* pval, int* mptype )
{
	// 配列 => 添字に対応する要素の functor 値を取り出す
	if ( pval->len[1] > 1 ) {
		int const idx = code_geti();
		code_index_int_rhs( pval, idx );

		*mptype = g_vtFunctor;
		return FunctorTraits::getValptr( pval );
	}

	// 呼び出し
	auto const pf = FunctorTraits::getValptr( pval );
	functor_t functor = *pf;

	// 非呼び出し添字 (バグへの対策)
	if ( *type == g_pluginType_call && *val == CallCmdId::NoCall ) {
		code_next();
		if ( code_isNextArg() ) puterror( HSPERR_SYNTAX );
		*mptype = g_vtFunctor;
		return pf;
	}

	if ( functor->getUsing() == 0 ) {
		puterror(HSPERR_ILLEGAL_FUNCTION);	// パラメータの値が異常
	}

	// 呼び出し
	{
		CCaller caller;

		Call_functorVar(functor, caller);

		// 返値を受け取る
		void* result = nullptr;
		*mptype = caller.getCallResult( &result );

		if ( *mptype == HSPVAR_FLAG_NONE || !result ) puterror( HSPERR_NORETVAL );
		return result;
	}
}

//------------------------------------------------
// 連想配列 : 参照 (左)
//------------------------------------------------
static void HspVarFunctor_arrayObject( PVal* pval )
{
	int const idx = code_geti();
	code_index_int_lhs( pval, idx );
	return;
}

/*
//------------------------------------------------
// 格納処理
//------------------------------------------------
static void HspVarFunctor_objectWrite( PVal* pval, void* data, int vtype )
{
	functor_t& functor = *FunctorTraits::getValptr( pval );
	functor = FunctorTraits::derefValptr(HspVarFunctor_cnv(data, vtype));

	// 連続代入
	code_assign_multi( pval );
	return;
}
//*/

//------------------------------------------------
// メソッド処理
//------------------------------------------------
static void HspVarFunctor_method(PVal* pval)
{
	char const* const psMethod = code_gets();

	if ( !strcmp(psMethod, "call") ) {
		CCaller caller;
		Call_functorVar( pval, caller );

	} else {
		puterror( HSPERR_ILLEGAL_FUNCTION );
	}
	return;
}

//------------------------------------------------
// 代入関数
//------------------------------------------------
static void  HspVarFunctor_set(PVal* pval, PDAT* pdat, PDAT const* in)
{
	auto& lhs = FunctorTraits::derefValptr(pdat);
	auto& rhs = FunctorTraits::derefValptr(in);

	lhs = rhs;
	return;
}

//------------------------------------------------
// 比較関数
//------------------------------------------------
//static
int HspVarFunctor_CmpI(PDAT* pdat, PDAT const* val)
{
	auto& lhs = FunctorTraits::derefValptr(pdat);
	auto& rhs = FunctorTraits::derefValptr(val);

	int const cmp = HspBool( lhs != rhs );

	g_hvpFunctor->aftertype = HSPVAR_FLAG_INT;
	return cmp;
}

//------------------------------------------------
// HspVarProc初期化関数
//------------------------------------------------
void HspVarFunctor_init(HspVarProc* p)
{
	g_vtFunctor = p->flag;
	g_hvpFunctor = p;

	p->GetPtr       = HspVarTemplate_GetPtr<functor_tag>;
	p->GetSize      = HspVarTemplate_GetSize<functor_tag>;
	p->GetUsing     = HspVarFunctor_getUsing;

	p->GetBlockSize = HspVarTemplate_GetBlockSize<functor_tag>;
	p->AllocBlock   = HspVarTemplate_AllocBlock<functor_tag>;

	p->Cnv          = HspVarFunctor_cnv;
	p->CnvCustom    = HspVarFunctor_cnvCustom;

	p->Alloc        = HspVarFunctor_alloc;
	p->Free         = HspVarFunctor_free;

	p->ArrayObjectRead = HspVarFunctor_arrayObjectRead;
	p->ArrayObject  = HspVarFunctor_arrayObject;
//	p->ObjectWrite  = HspVarFunctor_objectWrite;
	p->ObjectMethod = HspVarFunctor_method;

	p->Set          = HspVarFunctor_set;

//	p->AddI         = HspVarFunctor_addI;
//	p->SubI         = HspVarFunctor_subI;
//	p->MulI         = HspVarFunctor_mulI;
//	p->DivI         = HspVarFunctor_divI;
//	p->ModI         = HspVarFunctor_modI;

//	p->AndI         = HspVarFunctor_andI;
//	p->OrI          = HspVarFunctor_orI;
//	p->XorI         = HspVarFunctor_xorI;

	HspVarTemplate_InitCmpI_Equality< HspVarFunctor_CmpI >(p);

//	p->RrI          = HspVarFunctor_rrI;
//	p->LrI          = HspVarFunctor_lrI;

	p->vartype_name	= "functor_k";			// 型名
	p->version      = 0x001;				// VarType RuntimeVersion(0x100 = 1.0)
	p->support      = HSPVAR_SUPPORT_STORAGE
					| HSPVAR_SUPPORT_FLEXARRAY
					| HSPVAR_SUPPORT_ARRAYOBJ
					//| HSPVAR_SUPPORT_NOCONVERT
	                | HSPVAR_SUPPORT_VARUSE
					;						// サポート状況フラグ(HSPVAR_SUPPORT_*)
	p->basesize = FunctorTraits::basesize;	// 1つのデータのbytes / 可変長の時は-1
	return;
}

//##############################################################################
//                下請け関数
//##############################################################################
//------------------------------------------------
// functor オブジェクトを call する
// 
// @ 実引数はコードから取り出す
//------------------------------------------------
static void Call_functorVar( PVal* pval, CCaller& caller )
{
	Call_functorVar( *FunctorTraits::getValptr(pval), caller );
	return;
}

static void Call_functorVar( functor_t const& functor, CCaller& caller )
{
	caller.setFunctor(functor);
	caller.setArgAll();
	caller.call();
	return;
}
