// assoc - VarProc code

#include "CAssoc.h"
#include "vt_assoc.h"

#include "hsp3plugin_custom.h"
#include "mod_makepval.h"
#include "mod_argGetter.h"

#include "for_knowbug.var_assoc.h"

using namespace hpimod;

// 変数の定義
short g_vtAssoc;
HspVarProc* g_pHvpAssoc;

// 関数の宣言
extern PDAT* HspVarAssoc_GetPtr         ( PVal* pval) ;
extern int   HspVarAssoc_GetSize        ( PDAT const* pdat );
extern int   HspVarAssoc_GetUsing       ( PDAT const* pdat );
extern void* HspVarAssoc_GetBlockSize   ( PVal* pval, PDAT* pdat, int* size );
extern void  HspVarAssoc_AllocBlock     ( PVal* pval, PDAT* pdat, int  size );
extern void  HspVarAssoc_Alloc          ( PVal* pval, PVal const* pval2 );
extern void  HspVarAssoc_Free           ( PVal* pval);
extern void* HspVarAssoc_ArrayObjectRead( PVal* pval, int* mptype );
extern void  HspVarAssoc_ArrayObject    ( PVal* pval);
extern void  HspVarAssoc_ObjectWrite    ( PVal* pval, void* data, int vflag );
extern void  HspVarAssoc_ObjectMethod   ( PVal* pval);

static StAssocMapList* HspVarAssoc_GetMapList( CAssoc* src );	// void* user

//------------------------------------------------
// Core
//------------------------------------------------
static PDAT* HspVarAssoc_GetPtr( PVal* pval )
{
	return reinterpret_cast<PDAT*>(VtAssoc::getPtr(pval));
}

//------------------------------------------------
// Size
//------------------------------------------------
static int HspVarAssoc_GetSize( PDAT const* pdat )
{
	return VtAssoc::basesize;
}

//------------------------------------------------
// Using
//------------------------------------------------
static int HspVarAssoc_GetUsing( PDAT const* pdat )
{
	return static_cast<int>(*VtAssoc::asValptr(pdat) != nullptr);
}

//------------------------------------------------
// ブロックメモリ
//------------------------------------------------
static void* HspVarAssoc_GetBlockSize( PVal* pval, PDAT* pdat, int* size )
{
	*size = pval->size - ( ((char*)pdat) - ((char*)pval->pt) );
	return pdat;
}

static void HspVarAssoc_AllocBlock( PVal* pval, PDAT* pdat, int size )
{
	return;
}

//------------------------------------------------
// PValの変数メモリを確保する
//
// @ pval は未確保 or 解放済みの状態。
// @ pval2 != nullptr => pval2の内容を継承する。
//------------------------------------------------
static void HspVarAssoc_Alloc( PVal* pval, PVal const* pval2 )
{
	if ( pval->len[1] < 1 ) pval->len[1] = 1;		// 最低1要素は確保する
	size_t const cntElems = PVal_cntElems( pval );
	size_t const     size = cntElems * sizeof(CAssoc*);

	// バッファ確保
	auto const pt = reinterpret_cast<CAssoc**>(hspmalloc(size));
	std::memset(pt, 0, size);

	// 継承
	if ( pval2 ) {
		std::memcpy( pt, pval2->pt, pval2->size );
		hspfree( pval2->pt );
	}

	// 初期化
	for ( size_t i = 0; i < cntElems; ++ i ) {
		if ( !pt[i] ) {
			pt[i] = CAssoc::New();
			pt[i]->AddRef();
		}
	}

	// pval へ設定
	pval->flag   = g_vtAssoc;	// assoc の型タイプ値
	pval->mode   = HSPVAR_MODE_MALLOC;
	pval->size   = size;
	pval->pt     = reinterpret_cast<char*>(pt);
	pval->master = nullptr;		// 後で使う
	return;
}

//------------------------------------------------
// PValの変数メモリを解放する
//------------------------------------------------
static void HspVarAssoc_Free( PVal* pval )
{
	if ( pval->mode == HSPVAR_MODE_MALLOC ) {
		// 全ての要素を Release
		auto const pt = reinterpret_cast<CAssoc**>(pval->pt);
		size_t const cntElems = PVal_cntElems( pval );

		for ( size_t i = 0; i < cntElems; ++ i ) {
			CAssoc::Release( pt[i] );
		}

		// バッファを解放
		hspfree( pval->pt );
	}

	pval->pt   = nullptr;
	pval->mode = HSPVAR_MODE_NONE;
	return;
}

//------------------------------------------------
// 代入 (=)
// 
// @ 参照共有
//------------------------------------------------
static void HspVarAssoc_Set( PVal* pval, PDAT* pdat, void const* in )
{
	auto& dst = *VtAssoc::asValptr(pdat);
	auto& src = *VtAssoc::asValptr(in);

	if ( dst != src ) {
		CAssoc::Release( dst );
		dst = src;
		CAssoc::AddRef( dst );
	}

	g_pHvpAssoc->aftertype = g_vtAssoc;
	return;
}

/*
//------------------------------------------------
// マージ (+)
// 
// @ 左右の持つキーを併せ持つ Assoc を生成し、返す。
//------------------------------------------------
static void HspVarAssoc_AddI( PDAT* pdat, void const* val )
{

}
//*/

/*
//------------------------------------------------
// 比較関数 (参照同値性比較)
// 
// @ pdat は Assoc のテンポラリ変数の mpval->pt なので、
// @	値を代入すると、左辺の参照が1つ勝手に消えることになる。
// @	そのため、予め左辺の参照を Release しておく。
//------------------------------------------------
static void HspVarAssoc_EqI( PDAT* pdat, void const* val )
{
	CAssoc*& lhs = *(CAssoc**)pdat;
	CAssoc*& rhs = *(CAssoc**)val;

	if (lhs) lhs->Release();

	*(int*)pdat = (int)( (lhs == rhs) ? CAssoc::New() : NULL );
	g_pHvpAssoc->aftertype = HSPVAR_FLAG_INT;
	return;
}

static void HspVarAssoc_NeI( PDAT* pdat, void const* val )
{
	CAssoc*& lhs = *(CAssoc**)pdat;
	CAssoc*& rhs = *(CAssoc**)val;

	if (lhs) lhs->Release();

	*(int*)pdat = (int)( (lhs != rhs) ? CAssoc::New() : NULL );
	g_pHvpAssoc->aftertype = HSPVAR_FLAG_INT;
	return;
}
//*/

//------------------------------------------------
// Assoc 登録関数
//------------------------------------------------
void HspVarAssoc_Init( HspVarProc* p )
{
	g_pHvpAssoc     = p;
	g_vtAssoc       = p->flag;

	// 関数ポインタを登録
	p->GetPtr       = HspVarAssoc_GetPtr;
	p->GetSize      = HspVarAssoc_GetSize;
	p->GetUsing     = HspVarAssoc_GetUsing;

	p->Alloc        = HspVarAssoc_Alloc;
	p->Free         = HspVarAssoc_Free;
	p->GetBlockSize = HspVarAssoc_GetBlockSize;
	p->AllocBlock   = HspVarAssoc_AllocBlock;

	// 演算関数
	p->Set          = HspVarAssoc_Set;
//	p->AddI         = HspVarAssoc_AddI;
//	p->EqI          = HspVarAssoc_EqI;
//	p->NeI          = HspVarAssoc_NeI;

	// 連想配列用
	p->ArrayObjectRead = HspVarAssoc_ArrayObjectRead;	// 参照(右)
	p->ArrayObject     = HspVarAssoc_ArrayObject;		// 参照(左)
	p->ObjectWrite     = HspVarAssoc_ObjectWrite;		// 格納
	p->ObjectMethod    = HspVarAssoc_ObjectMethod;		// メソッド

	// 拡張データ
	p->user         = (char*)HspVarAssoc_GetMapList;

	// その他設定
	p->vartype_name = "assoc_k";		// タイプ名 (衝突しないように変な名前にする)
	p->version      = 0x001;			// runtime ver(0x100 = 1.0)

	p->support							// サポート状況フラグ(HSPVAR_SUPPORT_*)
		= HSPVAR_SUPPORT_STORAGE		// 固定長ストレージ
		| HSPVAR_SUPPORT_FLEXARRAY		// 可変長配列
	    | HSPVAR_SUPPORT_ARRAYOBJ		// 連想配列サポート
	    | HSPVAR_SUPPORT_NOCONVERT		// ObjectWriteで格納
	    | HSPVAR_SUPPORT_VARUSE			// varuse関数を適用
	    ;
	p->basesize = VtAssoc::basesize;	// size / 要素 (byte)
	return;
}

//#########################################################
//        連想配列用の関数群
//#########################################################
//------------------------------------------------
// 連想配列::添字処理
// 
// @ ( (idx...(0～4個), "キー", (idx...(内部変数)) )
// @ 内部変数の添字の処理は、呼び出し元が行う。
// @result: 内部変数 or nullptr(assoc自体が参照された)
//------------------------------------------------
template<bool bAsRhs>
static PVal* HspVarAssoc_IndexImpl( PVal* pval )
{
	if ( *type == TYPE_MARK && *val == ')' ) return nullptr;	// 添字状態を更新しない

	bool bKey = false;		// キーがあったか

	// [1] assoc 自体の添字と、キーを取得

	HspVarCoreReset( pval );		// 添字設定の初期化
	for ( int i = 0; i < (ArrayDimMax + 1) && code_isNextArg(); ++ i )
	{
		PVal pvalTemp;
		HspVarCoreCopyArrayInfo( &pvalTemp, pval );		// 添字状態を保存
		int const chk = code_getprm();
		HspVarCoreCopyArrayInfo( pval, &pvalTemp );		// 添字状態を復帰

		if ( chk == PARAM_DEFAULT ) {
			// 変数自体の参照 ( (, idxFullSlice) )
			if ( i == 0 && code_getdi(0) == assocIndexFullslice ) {
				pval->master = nullptr; return nullptr;
			}
			puterror(HSPERR_NO_DEFAULT);
		}
		if ( chk <= PARAM_END ) break;

		// int (最大4連続)
		if ( mpval->flag == HSPVAR_FLAG_INT ) {
			if ( pval->len[i + 1] <= 0 || i == 4 ) puterror( HSPERR_ARRAY_OVERFLOW );

			code_index_int( pval, *(int*)mpval->pt, !bAsRhs );	// 配列要素指定

		// str (キー)
		} else if ( mpval->flag == HSPVAR_FLAG_STR ) {
			bKey = true;
			++ pval->arraycnt;
			break;
		}
	}

	// [2] 参照先 (assoc or 内部変数) を決定

	if ( !bKey ) {		// キーなし => assoc 自体への参照
		pval->master = nullptr;
		return nullptr;
	}

	assert(mpval->flag == HSPVAR_FLAG_STR);
	static CAssoc::Key_t stt_key;
	stt_key = (char*)mpval->pt;		// 既製の変数に格納した方が高速 (一時オブジェクトを作らないため)

	auto const pAssoc = VtAssoc::getPtr(pval);
	if ( !*pAssoc ) {
		(*pAssoc) = CAssoc::New();
		(*pAssoc)->AddRef();
	}

	PVal* const pvInner = (!bAsRhs
		? (*pAssoc)->At( stt_key )		// 左辺値 => 自動拡張あり
		: (*pAssoc)->AtSafe( stt_key )	// 右辺値 => 自動拡張なし
	);

	if ( bAsRhs && !pvInner ) puterror( HSPERR_ARRAY_OVERFLOW );

	pval->master = pvInner;
	return pvInner;
}

//------------------------------------------------
// 連想配列::参照 (左辺値)
//------------------------------------------------
static void HspVarAssoc_ArrayObject( PVal* pval )
{
	PVal* const pvInner = HspVarAssoc_IndexImpl<false>( pval );
	if ( !pvInner ) return;

	// [3] 内部変数の添字を処理
	if ( code_isNextArg() ) {			// 添字が続く場合
		code_expand_index_lhs( pvInner );
	} else {
		if ( PVal_supportArray(pvInner) && !(pvInner->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {	// 標準配列サポート
			HspVarCoreReset( pvInner );		// 添字状態の初期化だけしておく
		}
	}

	return;
}

//------------------------------------------------
// 連想配列::参照 (右辺値)
//------------------------------------------------
static void* HspVarAssoc_ArrayObjectRead( PVal* pval, int* mptype )
{
	PVal* const pvInner = HspVarAssoc_IndexImpl<true>( pval );

	// assoc 自体の参照
	if ( !pvInner ) {
		*mptype = g_vtAssoc;
		return VtAssoc::getPtr( pval );
	}

	// [3] 内部変数の添字を処理
	if ( code_isNextArg() ) {			// 添字が続く場合
		return code_expand_index_rhs( pvInner, mptype );
	} else {
		if ( PVal_supportArray(pvInner) && !(pvInner->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {	// 標準配列サポート
			HspVarCoreReset( pvInner );		// 添字状態の初期化だけしておく
		}

		*mptype = pvInner->flag;
		return getHvp( pvInner->flag )->GetPtr( pvInner );
	}
}

//------------------------------------------------
// 連想配列::格納
//------------------------------------------------
static void HspVarAssoc_ObjectWrite( PVal* pval, void* data, int vflag )
{
	PVal* const pvInner = VtAssoc::getMaster(pval);

	// assoc への代入
	if ( !pvInner ) {
		if ( vflag != g_vtAssoc ) puterror( HSPERR_INVALID_ARRAYSTORE );	// 右辺の型が不一致

		HspVarAssoc_Set( pval, HspVarAssoc_GetPtr(pval), data );
		code_assign_multi( pval );				// 連続代入の処理

	// 内部変数を参照している場合
	} else {
		int const fUserElem = pvInner->support & CAssoc::HSPVAR_SUPPORT_USER_ELEM;

		PVal_assign( pvInner, data, vflag );	// 内部変数への代入処理
		code_assign_multi( pvInner );

		pvInner->support |= fUserElem;
	}

	return;
}

//------------------------------------------------
// メソッド呼び出し
// 
// @ 内部変数の型で提供されているメソッドを使う
//------------------------------------------------
static void HspVarAssoc_ObjectMethod(PVal* pval)
{
	PVal* const pvInner = VtAssoc::getMaster(pval);
	if ( !pvInner ) puterror( HSPERR_UNSUPPORTED_FUNCTION );

	// 内部変数の処理に転送
	getHvp(pvInner->flag)->ObjectMethod( pvInner );
	return;
}

//------------------------------------------------
// すべてのキーを列挙する
// 
// @ for knowbug
// @ リストの削除権限は呼び出し元にある。
//------------------------------------------------
// [[deprecated]]
static StAssocMapList* HspVarAssoc_GetMapList( CAssoc* src )
{
	if ( !src || src->Empty() ) return nullptr;

	StAssocMapList* head = nullptr;
	StAssocMapList* tail = nullptr;

	for ( auto iter : *src ) {
		auto const list = reinterpret_cast<StAssocMapList*>(hspmalloc(sizeof(StAssocMapList)));

		lstrcpy( list->key, iter.first.c_str() );
		list->pval = iter.second;

		// 連結
		if ( !head ) {
			head = list;
		} else {
			tail->next = list;
		}
		tail = list;
	}
	if ( tail ) tail->next = nullptr;

	return head;
}

//------------------------------------------------
// knowbug での拡張型表示に対応する
//------------------------------------------------
#include "knowbug/knowbugForHPI.h"

EXPORT void WINAPI knowbugVsw_addValueAssoc(vswriter_t _w, char const* name, void const* ptr)
{
	auto const kvswm = knowbug_getVswMethods();
	if ( !kvswm ) return;

	auto const src = *reinterpret_cast<CAssoc* const*>(ptr);

	// null
	if ( !src) {
		kvswm->catLeafExtra(_w, name, "null_assoc");
	}

	// 要素なし
	if ( src->Empty() ) {
		kvswm->catLeafExtra(_w, name, "empty_assoc");
		return;
	}

	for ( auto iter : *src ) {
		auto const& key = iter.first;
		auto const pval = iter.second;

		if ( kvswm->isLineformWriter(_w) ) {
			// pair: 「key: value...」
			kvswm->catNodeBegin(_w, nullptr, (key + ": ").c_str());
			kvswm->addVar(_w, nullptr, pval);
			kvswm->catNodeEnd(_w, "");
		} else {
			kvswm->addVar(_w, key.c_str(), pval);
		}

	}
	return;

#if 0
	if ( !ptr ) {
		knowbugVsw_catLeafExtra(_w, name, "null_assoc");
		return;
	}

	auto const hvp = hpimod::seekHvp(assoc_vartype_name);
	StAssocMapList* const head = (reinterpret_cast<GetMapList_t>(hvp->user))(src);

	// 要素なし
	if ( !head ) {
		knowbugVsw_catLeafExtra(_w, name, "empty_assoc");
		return;
	}

	// 全キーのリスト
	knowbugVsw_catNodeBegin(_w, name, "[");
	{
		// 列挙
		for ( StAssocMapList* list = head; list != nullptr; list = list->next ) {
			if ( knowbugVsw_isLineformWriter(_w) ) {
				// pair: 「key: value...」
				knowbugVsw_catNodeBegin(_w, CStructedStrWriter::stc_strUnused,
					strf("%s: ", list->key).c_str());
				knowbugVsw_addVar(_w, CStructedStrWriter::stc_strUnused, list->pval);
				knowbugVsw_catNodeEnd(_w, "");
			} else {
				knowbugVsw_addVar(_w, list->key, list->pval);
			}
			//	dbgout("%p: key = %s, pval = %p, next = %p", list, list->key, list->pval, list->next );
		}

		// リストの解放
		for ( StAssocMapList* list = head; list != nullptr; ) {
			StAssocMapList* const next = list->next;
			exinfo->HspFunc_free(list);
			list = next;
		}
	}
	knowbugVsw_catNodeEnd(_w, "]");
	return;
#endif

}