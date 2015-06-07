// PVal の独自管理

#include "mod_makepval.h"

namespace hpimod {

static PVal* PVal_initDefault(vartype_t vt);

//------------------------------------------------
// PVal構造体の初期化
// 
// @ (pval == nullptr) => 何もしない。
// @prm pval: 不定値でも可
//------------------------------------------------
void PVal_init(PVal* pval, vartype_t vtype)
{
	if ( !pval ) return;

	pval->flag = vtype;
	pval->mode = HSPVAR_MODE_NONE;
	PVal_alloc( pval, nullptr, vtype );
	return;
}

//------------------------------------------------
// もっとも簡単で有効なPVal構造体にする
// 
// @ (pval == nullptr) => 何もしない。
// @ (vtype == 無効) => pval->flag の型に初期化する。
// @ HspVarCoreDim の代わり (配列添字は指定できないが)。
//------------------------------------------------
void PVal_alloc(PVal* pval, PVal* pval2, vartype_t vtype)
{
	if ( !pval ) return;
	if ( vtype <= HSPVAR_FLAG_NONE ) vtype = pval->flag;
	if ( vtype <= HSPVAR_FLAG_NONE ) return;

	HspVarProc* const vp = getHvp( vtype );

	// pt が確保されている場合、解放する
	if ( pval->flag != HSPVAR_FLAG_NONE && pval->mode == HSPVAR_MODE_MALLOC ) {
		PVal_free( pval );
	}

	// 確保処理
	memset( pval, 0x00, sizeof(PVal) );
	pval->flag     = vtype;
	pval->mode     = HSPVAR_MODE_NONE;
	pval->support |= vp->support;
	vp->Alloc( pval, pval2 );
	return;
}

//------------------------------------------------
// PVal構造体を簡単に初期化する
// 
// @ 最も簡単な形で確保される。
// @ HspVarCoreClear の代わり。
//------------------------------------------------
void PVal_clear(PVal* pval, vartype_t vtype)
{
	PVal_alloc( pval, nullptr, vtype );
}

//------------------------------------------------
// PVal 構造体の中身を解放する
// 
// @ (pval == nullptr) => 何もしない。
// @ pval ポインタ自体は破壊されない。
//------------------------------------------------
void PVal_free(PVal* pval)
{
	if ( !pval ) return;

	HspVarProc* const vp = getHvp( pval->flag );
	vp->Free( pval );

	pval->support &= ~vp->support;		// meta でない support フラグを取り除く
	return;
}

//------------------------------------------------
// 既定値を表す PVal 構造体を初期化
// @private
// @ vt は必ず有効な値 (str ～ int)。
//------------------------------------------------
static PVal* PVal_initDefault(vartype_t vt)
{
	static PVal** stt_pDefPVal   = nullptr;
	static int    stt_cntDefPVal = 0;

	// stt_pDefPVal の拡張
	if ( stt_cntDefPVal <= vt ) {
		int cntNew = vt + 1;

		if ( !stt_pDefPVal ) {
			stt_pDefPVal = reinterpret_cast<PVal**>(hspmalloc( cntNew * sizeof(PVal*) ));

		} else {
			stt_pDefPVal = reinterpret_cast<PVal**>(hspexpand(
				reinterpret_cast<char*>( stt_pDefPVal ),
				cntNew * sizeof(PVal*)
			));
		}

		// 拡張分を nullptr で初期化する
		for( int i = stt_cntDefPVal; i < cntNew; ++ i ) {
			stt_pDefPVal[i] = nullptr;
		}

		stt_cntDefPVal = cntNew;
	}

	// 未初期化の場合は、PVal のメモリを確保し、初期化する
	if ( !stt_pDefPVal[vt] ) {
		stt_pDefPVal[vt] = reinterpret_cast<PVal*>(hspmalloc( sizeof(PVal) ));
		PVal_init( stt_pDefPVal[vt], vt );
	}
	return stt_pDefPVal[vt];
}

//------------------------------------------------
// 既定値を表す PVal 構造体へのポインタを得る
// 
// @ vt が不正な場合、nullptr を返す。
//------------------------------------------------
PVal* PVal_getDefault( vartype_t vt )
{
	if ( vt <= HSPVAR_FLAG_NONE ) {
		return nullptr;

	} else {
		return PVal_initDefault( vt );
	}
}

//##########################################################
//        変数情報の取得
//##########################################################
#if 0
// basis にある
//------------------------------------------------
// 変数の要素の総数を返す
//------------------------------------------------
size_t PVal_cntElems( PVal const* pval )
{
	int cntElems = 1;

	// 要素数を調べる
	for ( unsigned int i = 0; i < ArrayDimMax; ++ i ) {
		if ( pval->len[i + 1] ) {
			cntElems *= pval->len[i + 1];
		}
	}

	return cntElems;
}
#endif

//------------------------------------------------
// 変数のサイズを返す
// 
// @ pval->offset を見る。
// @ 固定長型なら HspVarProc::basesize を、
// @	可変長型なら、指定要素のサイズを。
//------------------------------------------------
size_t PVal_size( PVal const* pval )
{
	auto const vp = getHvp( pval->flag );

	if ( vp->basesize < 0 ) {
		return vp->GetSize( reinterpret_cast<PDAT const*>(pval->pt) );
	} else {
		return vp->basesize;
	}
}

//------------------------------------------------
// 変数から実体ポインタを得る
// 
// @ pval->offset を見る。
//------------------------------------------------
PDAT* PVal_getptr( PVal* pval )
{
	return getHvp(pval->flag)->GetPtr( pval );
}

PDAT* PVal_getptr( PVal* pval, APTR aptr )
{
	pval->offset = aptr;
	if ( pval->arraycnt == 0 ) pval->arraycnt = 1;
	return PVal_getptr(pval);
}

//##########################################################
//        変数に対する操作
//##########################################################
//------------------------------------------------
// PValへ値を格納する (汎用)
// 
// @ pval の添字状態を参照する。
//------------------------------------------------
void PVal_assign( PVal* pval, PDAT const* data, vartype_t vtype )
{
	// 添字あり => ObjectWrite
	if ( (pval->support & HSPVAR_SUPPORT_NOCONVERT) && (pval->arraycnt != 0) ) {
		getHvp(pval->flag)->ObjectWrite(pval, data, vtype);

	// 通常の代入
	} else {
		code_setva(pval, pval->offset, vtype, data);
	}
	return;
}

//------------------------------------------------
// 相互代入
//------------------------------------------------
static void PVal_assign_mutual_impl( PVal* pvLhs, PVal* pvRhs, PVal* pvTmp );

void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs )
{
	PVal vTmp = {0};
	PVal_assign_mutual( pvLhs, pvRhs, &vTmp );
}

void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs )
{
	PVal vTmp = {0};
	PVal_assign_mutual( pvLhs, pvRhs, apLhs, apRhs, &vTmp );
}

void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, PVal* pvTmp )
{
	if ( pvLhs == pvRhs ) return;
	PVal_assign_mutual( pvLhs, pvRhs, pvLhs->offset, pvRhs->offset, pvTmp );
	return;
}

void PVal_assign_mutual( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs, PVal* pvTmp )
{
	if ( pvLhs == pvRhs ) {
		PVal vLhs = *pvLhs; vLhs.offset = apLhs;		// 添字状態を保存するため
		PVal vRhs = *pvRhs; vRhs.offset = apRhs;
		PVal_assign_mutual_impl( &vLhs, &vRhs, pvTmp );
	} else {
		pvLhs->offset = apLhs;
		pvRhs->offset = apRhs;
		PVal_assign_mutual_impl( pvLhs, pvRhs, pvTmp );
	}
	return;
}

void PVal_assign_mutual_impl( PVal* pvLhs, PVal* pvRhs, PVal* pvTmp )
{
//	assert( pvLhs != pvRhs );
	PVal_init( pvTmp, pvLhs->flag );
	{
		PVal_assign( pvTmp, PVal_getptr(pvLhs), pvLhs->flag );
		PVal_assign( pvLhs, PVal_getptr(pvRhs), pvRhs->flag );
		PVal_assign( pvRhs, pvTmp->pt, pvTmp->flag );
	}
	PVal_free( pvTmp );
	return;
}

//------------------------------------------------
// PValの複写
// 
// @ 全要素を複写する。
//------------------------------------------------
void PVal_copy(PVal* pvDst, PVal* pvSrc)
{
	if ( pvDst == pvSrc ) return;

	size_t const cntElems = PVal_cntElems(pvSrc);

	// pvDst を確保する
	exinfo->HspFunc_dim(
		pvDst, pvSrc->flag, 0, pvSrc->len[1], pvSrc->len[2], pvSrc->len[3], pvSrc->len[4]
	);

	// 単純変数 => 第一要素を代入するのみ
	if ( cntElems == 1 ) {
		PVal_assign( pvDst, pvSrc->pt, pvSrc->flag );

	// 連続代入処理 => すべての要素を代入する
	} else {
		auto const hvpSrc = getHvp( pvSrc->flag );

		pvDst->arraycnt = 1;
		for ( size_t i = 0; i < cntElems; ++ i ) {
			pvDst->offset = i;
			pvSrc->offset = i;

			PVal_assign(pvDst, hvpSrc->GetPtr(pvSrc), pvSrc->flag);
		}
	}
	return;
}

//------------------------------------------------
// 変数値の交換
//------------------------------------------------
void PVal_swap( PVal* pvLhs, PVal* pvRhs )
{
	PVal_swap( pvLhs, pvRhs, pvLhs->offset, pvRhs->offset );
}

void PVal_swap( PVal* pvLhs, PVal* pvRhs, APTR apLhs, APTR apRhs )
{
	PVal   vTmp = {0};
	PVal* pvTmp = &vTmp;

	if ( pvLhs->arraycnt != 0 || pvRhs->arraycnt != 0 ) {
		// @ 一方が配列要素 => 代入による交換

		PVal_assign_mutual( pvLhs, pvRhs, apLhs, apRhs, pvTmp );

	} else {
		// @ 両方とも変数 => PVal 自体の交換 (meta な support フラグは維持する)
		int const supportMetaLhs = PVal_supportMeta( pvLhs );
		int const supportMetaRhs = PVal_supportMeta( pvRhs );

		*pvTmp = *pvLhs;
		*pvLhs = *pvRhs;
		*pvRhs = *pvTmp;

		pvRhs->support |= supportMetaLhs;
		pvLhs->support |= supportMetaRhs;
	}

	return;
}

//------------------------------------------------
// 変数をクローンにする
// 
// @ PVal::offset を参照する。
// @ HspVarCoreDup, HspVarCoreDupPtr
//------------------------------------------------
void PVal_clone( PVal* pvDst, PVal* pvSrc, APTR aptrSrc )
{
	HspVarProc* const vp = getHvp( pvSrc->flag );

	if ( aptrSrc >= 0 ) pvSrc->offset = aptrSrc;
	PDAT* const pSrc = vp->GetPtr(pvSrc);

	int size;								// クローンにするサイズ
	vp->GetBlockSize( pvSrc, pSrc, &size );

	// 実体ポインタからクローンを作る
	PVal_clone( pvDst, pSrc, pvSrc->flag, size );
	return;
}

void PVal_clone( PVal* pval, PDAT* ptr, int vtype, int size )
{
	PVal_free( pval );

	HspVarProc* const vp = getHvp(vtype);

	pval->pt   = ptr;
	pval->flag = vtype;
	pval->size = size;
	pval->mode = HSPVAR_MODE_CLONE;
	pval->len[0] = 1;

	if ( vp->basesize < 0 ) {
		pval->len[1] = 1;
	} else {
		pval->len[1] = size / vp->basesize;
	}
	pval->len[2] = 0;
	pval->len[3] = 0;
	pval->len[4] = 0;
	pval->offset = 0;
	pval->arraycnt = 0;
	pval->support |= HSPVAR_SUPPORT_STORAGE;
	return;
}

//------------------------------------------------
// 変数をクローンにする
// 
// @ 変数に対するより強力なクローン。
//------------------------------------------------
void PVal_cloneVar( PVal* pvDst, PVal* pvSrc, APTR aptrSrc )
{
	PVal_free( pvDst );
	int const supportMeta = pvDst->support;	// meta な support フラグは維持する (PVal_free によって meta でない support フラグは取り除かれている)

	// 配列へのクローン
	if ( pvSrc->arraycnt == 0 && aptrSrc < 0 ) {
		*pvDst = *pvSrc;					// まるごとコピー

	// 単一要素へのクローン
	} else {
		if ( aptrSrc >= 0 ) pvSrc->offset = aptrSrc;
		HspVarProc const* const vp  = getHvp(pvSrc->flag);
		PDAT const*       const ptr = vp->GetPtr( pvSrc );

		std::memset( pvDst, 0x00, sizeof(PVal) );
		pvDst->flag    = pvSrc->flag;
		pvDst->len[1]  = 1;
		pvDst->pt      = const_cast<PDAT*>(ptr);
		pvDst->size    = vp->GetSize( ptr );
		pvDst->master  = pvSrc->master;
	}

	pvDst->mode    = HSPVAR_MODE_CLONE;		// クローンということにする
	pvDst->support = PVal_supportNotmeta(pvSrc) | supportMeta;
	return;
}

//------------------------------------------------
// 値をシステム変数に代入する
//------------------------------------------------
void SetResultSysvar(PDAT const* data, vartype_t vtype)
{
	if ( !data ) return;

	ctx->retval_level = ctx->sublev;

	switch ( vtype ) {
		case HSPVAR_FLAG_INT:
			ctx->stat = VtTraits::derefValptr<vtInt>(data);
			break;

		case HSPVAR_FLAG_STR:
			strcpy_s(
				ctx->refstr, HSPCTX_REFSTR_MAX,
				VtTraits::asValptr<vtStr>(data)
			);
			break;

		case HSPVAR_FLAG_DOUBLE:
			ctx->refdval = VtTraits::derefValptr<vtDouble>(data);
			break;

		default:
			puterror( HSPERR_TYPE_MISMATCH );
	}
}

//------------------------------------------------
// 実体ポインタを型変換する
//------------------------------------------------
PDAT const* Valptr_cnvTo( PDAT const* pValue, vartype_t vtSrc, vartype_t vtDst )
{
	return (PDAT const*)(
		( vtSrc < HSPVAR_FLAG_USERDEF )
			? getHvp(vtDst)->Cnv( pValue, vtSrc )
			: getHvp(vtSrc)->CnvCustom( pValue, vtDst )
	);
}

//------------------------------------------------
// meta な support フラグを取り出す
// 
// @ その変数の型の HspVarProc::support が持っているフラグは
// @	その変数に固有のフラグであり、
// @	そうでないフラグはその PVal に固有のフラグである、と解釈し、
// @	後者のフラグを「meta な support フラグ」と呼ぶ。
// @ex: HSPVAR_SUPPORT_TEMPVAR
//------------------------------------------------
int PVal_supportMeta( PVal* pval )
{
	return pval->support &~ getHvp(pval->flag)->support;
}

int PVal_supportNotmeta( PVal* pval )
{
	return pval->support & getHvp(pval->flag)->support;
}

//------------------------------------------------
// 配列サポートか否か
//------------------------------------------------
bool PVal_supportArray( PVal* pval )
{
	return (pval->support & ( HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY | HSPVAR_SUPPORT_ARRAYOBJ )) != 0;
}

} // namespace hpimod
