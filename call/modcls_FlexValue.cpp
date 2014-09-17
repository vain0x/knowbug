// Call(ModCls) - FlexValue
#if 0
#include "hsp3plugin_custom.h"
#include "mod_makePVal.h"

#include "CCaller.h"
#include "CCall.h"
#include "Functor.h"

#include "modcls_FlexValue.h"
#include "vt_structWrap.h"

static int const FLEXVAL_TYPE_ALLOC_EX = 9;		// FLEXVAL_TYPE_ALLOC とは異なる値

// 特殊なメンバへの参照 (self.ptr の末尾)
static int& FlexValueEx_Counter( FlexValue const& self );
static int& FlexValueEx_TmpFlag( FlexValue const& self );

void FlexValue_Dbgout( FlexValue& self )
{
	dbgout( "struct id: %d, ptr: 0x%08x, size: %d, type: %d, <%d>", FlexValue_SubId(self), (int)self.ptr, self.size, self.type, self.ptr ? FlexValue_Counter(self) : 0 );
}

//------------------------------------------------
// FlexValue 構築
// 
// @ コードから ctor の引数が取り出せる状態。
// @ ctor の引数は thismod の PVal を参照渡しした方が効率がよいので、
// @	奇妙ではあるが thismod の pval, aptr をもらっておく。
// @	もらえなかった場合は普通に値渡しする。
// @ コンストラクタの返値は無視する。
//------------------------------------------------
void FlexValue_Ctor( FlexValue& self, stdat_t modcls )
{
	FlexValue_Ctor( self, modcls, nullptr, 0 );
}

void FlexValue_Ctor( FlexValue& self, stdat_t modcls, PVal* pval, APTR aptr )
{
	stprm_t const pStPrm = STRUCTDAT_getStPrm(modcls);

	FlexValue_CtorWoCtorCalling( self, modcls );

	// ctor 実行
	if ( pStPrm->offset != -1 ) {
		CCaller caller;
		caller.setFunctor( AxCmd::make(TYPE_MODCMD, pStPrm->offset) );	// (#modinit)
		if ( pval ) {
			caller.addArgByRef( pval ); pval->offset = aptr;	// thismod
		} else {
			caller.addArgByVal( &self, HSPVAR_FLAG_STRUCT );
		}
		caller.setArgAll();

		caller.call();
	}
	return;
}

void FlexValue_CtorWoCtorCalling( FlexValue& self, stdat_t modcls )
{
	stprm_t const pStPrm = STRUCTDAT_getStPrm(modcls);
	if ( pStPrm->mptype != MPTYPE_STRUCTTAG ) puterror( HSPERR_STRUCT_REQUIRED );

	// 新要素初期化
	{
		// 参照カウンタ、テンポラリフラグの分だけ大きく確保する
		size_t const size = modcls->size + sizeof(int) * 2;	

		self.type      = FLEXVAL_TYPE_ALLOC_EX;
		self.myid      = 0;
		self.customid  = modcls->prmindex;
		self.clonetype = 1;
		self.size      = size;
		self.ptr       = hspmalloc(size);

		FlexValueEx_TmpFlag( self ) = 0;
		FlexValueEx_Counter( self ) = 1;
	}

	// メンバ変数の初期化
	for ( int i = 0; i < modcls->prmmax; ++ i ) {
		void* const out = Prmstack_getMemberPtr(self.ptr, &pStPrm[i]);
		switch ( pStPrm[i].mptype ) {
			case MPTYPE_STRUCTTAG: break;
			case MPTYPE_LOCALVAR:
				PVal_init( reinterpret_cast<PVal*>(out), HSPVAR_FLAG_INT );
				break;
			default:
				puterror( HSPERR_UNKNOWN_CODE );
		}
	}
	return;
}

//------------------------------------------------
// FlexValue 解体
// 
// @ code_delstruct
// @ デストラクタの返値は無視する。
// @ デストラクタを呼ぶ場合、その前後で mpval が変化しないように
// @	mpval のポインタとその値を保存する。
//------------------------------------------------
void FlexValue_Dtor( FlexValue& self )
{
	if ( !FlexValueEx_Valid(self) ) return;

	DbgArea { if ( FlexValueEx_Counter(self) != 0 ) dbgout("参照カウンタが 0 でないのに Dtor が呼ばれた。"); }

	// デストラクタが呼び直されないように、参照カウンタを書き換えておく。
	FlexValueEx_Counter(self) = FLEXVAL_COUNTER_DTORING;

	stdat_t const modcls = FlexValue_ModCls(self);

	// dtor 実行
	if ( modcls->otindex != 0 ) {
		// mpval の値を保存しておく
		PVal* const mpval_bak = mpval;

		// thismod 用の変数
		PVal _pvTmp { };
		PVal* const pvTmp = &_pvTmp;
			pvTmp->flag   = HSPVAR_FLAG_STRUCT;
			pvTmp->mode   = HSPVAR_MODE_CLONE;
			pvTmp->pt     = ModCls::StructTraits::asPDAT(&self);
			pvTmp->len[1] = 1;

		CCaller caller;
		caller.setFunctor(AxCmd::make(TYPE_MODCMD, modcls->otindex));			// (#modterm)
		caller.addArgByRef( pvTmp );
		caller.addArgByVal( mpval->pt, mpval->flag );	// 保存するために引数に入れておく (このために、デストラクタは可変長引数扱いにしてある)
		caller.call();

		mpval = mpval_bak;		// mpval を restore

		PVal* const pvArg1 = caller.getCall().getArgPVal(1);
		PVal_assign(mpval, pvArg1->pt, pvArg1->flag);
	}

	// member 解放
	{
		void* const members = self.ptr;

		stprm_t const pStPrm = FlexValue_getModuleTag(&self);
		for ( int i = 0; i < modcls->prmmax; ++i ) {
			void* const out = Prmstack_getMemberPtr(members, pStPrm);
			switch ( pStPrm[i].mptype ) {
				case MPTYPE_STRUCTTAG: break;
				case MPTYPE_LOCALVAR:
					PVal_free(reinterpret_cast<PVal*>(out));
					break;
				default:
					puterror(HSPERR_UNKNOWN_CODE);
			}
		}

		hspfree(members);
	}

	// null clear
	FlexValue_NullClear(self);
	return;
}

//------------------------------------------------
// FlexValue 複写
//------------------------------------------------
void FlexValue_Copy( FlexValue& dst, FlexValue const& src )
{
	if ( dst.ptr == src.ptr ) return;

	FlexValue_Release(dst);
	dst = src;
	FlexValue_AddRef(dst);
	return;
}

//------------------------------------------------
// FlexValue 移動
//------------------------------------------------
void FlexValue_Move( FlexValue& dst, FlexValue& src )
{
	FlexValue_Release( dst );
	dst = src;
	FlexValue_NullClear( src );
	return;
}

//------------------------------------------------
// FlexValue 参照カウンタ増減
//------------------------------------------------
void FlexValue_AddRef( FlexValue const& self )
{
	if ( !FlexValueEx_Valid(self) ) return;

	int& cnt = FlexValueEx_Counter(self);
	
	dbgout("%08X addref(++); %d -> %d", (int)self.ptr, cnt, cnt + 1);

	if ( cnt != FLEXVAL_COUNTER_DTORING ) ++ cnt;
	return;
}

void FlexValue_Release( FlexValue const& self )
{
	if ( !FlexValueEx_Valid(self) ) return;

	int& cnt = FlexValueEx_Counter(self);

	dbgout("%08X release(--); %d -> %d", (int)self.ptr, cnt, cnt - 1);
	
	if ( cnt != FLEXVAL_COUNTER_DTORING ) {
		if ( (--cnt) == 0 ) {		// (cnt < 0) => dtor 呼ばない
			FlexValue_Dtor(const_cast<FlexValue&>(self));
		}
	}
	return;
}

void FlexValue_DelRef( FlexValue& self )
{
	FlexValue_Release(self);
	FlexValue_NullClear(self);
	return;
}

//------------------------------------------------
// FlexValue 0 クリア
// 
// @ 参照カウンタを無視するので危険。
//------------------------------------------------
void FlexValue_NullClear( FlexValue& self )
{
	std::memset( &self, 0x00, sizeof(FlexValue) );
}

//------------------------------------------------
// FlexValue メンバ取得
//------------------------------------------------
bool FlexValue_IsNull( FlexValue const& self )
{
	return ( self.ptr == nullptr );
}

// ModCls が生成した nullmod でない FlexValue であるか
bool FlexValueEx_Valid( FlexValue const& self )
{
	return (self.type == FLEXVAL_TYPE_ALLOC_EX);
}

int FlexValue_SubId( FlexValue const& self )
{
	return ( !FlexValue_IsNull(self) ? FlexValue_getModuleTag(&self)->subid : -1 );
}

stdat_t FlexValue_ModCls( FlexValue const& self )
{
	return ( !FlexValue_IsNull(self) ? FlexValue_getModule(&self) : nullptr );
}

char const* FlexValue_ClsName( FlexValue const& self )
{
	return ModCls_Name( FlexValue_ModCls( self ) );
}

// 拡張メンバ (どちらも mutable 扱い)
int& FlexValueEx_Counter( FlexValue const& self )	// 参照カウンタへのポインタ (バッファ末尾)
{
	return *reinterpret_cast<int*>( &static_cast<char*>(self.ptr)[self.size - sizeof(int) * 1] );
}
int FlexValue_Counter( FlexValue const& self ) { return FlexValueEx_Counter(self); }

int& FlexValueEx_TmpFlag( FlexValue const& self )
{
	return *reinterpret_cast<int*>( &static_cast<char*>(self.ptr)[self.size - sizeof(int) * 2] );
}

//------------------------------------------------
// modinst 型の値を取り出す
// 
// @ code_gets などと同様に、次の呼び出し時に
// @	ポインタの先の値が変わるので注意。
// @ nullptr は返却しない。
//------------------------------------------------
FlexValue* code_get_modinst_impl( FlexValue* def, bool const bDefault )
{
	int prm = code_getprm();
	if ( prm <= PARAM_END ) {
		if ( prm == PARAM_DEFAULT && bDefault ) return def;
		puterror( HSPERR_NO_DEFAULT );
	}
	if ( mpval->flag != HSPVAR_FLAG_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );
	return ModCls::StructTraits::asValptr(mpval->pt);
}

FlexValue* code_get_modinst()
{
	return code_get_modinst_impl( nullptr, false );
}

FlexValue* code_get_modinst( FlexValue* def )
{
	return code_get_modinst_impl( def, true );
}

//------------------------------------------------
// 一時オブジェクトのロック
// 
// @ mpval だけが所有するようなインスタンスは
// @	式の実行中に死亡しうるので、ロックする。
//------------------------------------------------
/*
#include <deque>
static std::deque<FlexValue*> stt_modinst_locker;

void FlexValue_Lock( FlexValue& self )
{
	stt_modinst_locker.push_back( &self );
	FlexValue_AddRef( self );		// lock!
	return;
}

void FlexValue_LockRelease()
{
	for each ( auto it in stt_modinst_locker ) {
		FlexValue_Release( *it );
	}
	return;
}
//*/

//------------------------------------------------
// 一時オブジェクトのフラグ
// 
// @ HSP の計算スタックに、変数ではなく FlexValue (右辺値)自体が積まれる場合、
// @	そのインスタンスがスタックに所有されていると考えて、参照カウンタを増やす。
// @	さらに、Tmp フラグを立てておくことで、スタックから降ろされるときに、
// @	それがスタックに所有されていたか否かが判断でき、安全に破棄できる。
//------------------------------------------------
void FlexValue_AddRefTmp( FlexValue const& self )
{
	if ( !FlexValueEx_Valid(self) ) return;

	FlexValueEx_TmpFlag( self ) ++;
	FlexValue_AddRef( self );
	return;
}

// 一時オブジェクトなら解放する
void FlexValue_ReleaseTmp( FlexValue const& self )
{
	if ( FlexValueEx_Valid(self) && FlexValueEx_TmpFlag(self) > 0 ) {
		assert(FlexValueEx_TmpFlag(self) == 1);
		FlexValueEx_TmpFlag(self)--;
		FlexValue_Release(self);
	}
	return;
}

bool FlexValue_IsTmp( FlexValue const& self )
{
	return ( FlexValueEx_Valid(self) ? FlexValueEx_TmpFlag(self) > 0 : false );
}
#endif
