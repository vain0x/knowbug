// assoc - Command code

#include "vt_assoc.h"
#include "cmd_assoc.h"
#include "CAssoc.h"

#include "mod_makepval.h"
#include "mod_argGetter.h"
#include "mod_func_result.h"

using namespace hpimod;

static CAssoc* g_pAssocResult = nullptr;	// 返却値を所有する

//------------------------------------------------
// assoc 型の値を受け取る
// 
// @ mpval は assoc 型となる。
//------------------------------------------------
CAssoc* code_get_assoc()
{
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	if ( mpval->flag != g_vtAssoc ) puterror( HSPERR_TYPE_MISMATCH );
	return *VtAssoc::asValptr(mpval->pt);
}

//------------------------------------------------
// assoc の内部変数を受け取る
//
// @ 内部変数を指す添字がついていなければ、nullptr。
//------------------------------------------------
PVal* code_get_assoc_pval()
{
	PVal* const pval = hpimod::code_get_var();
	return VtAssoc::getMaster(pval);
}

//------------------------------------------------
// assoc 型の値を返却する
//------------------------------------------------
int SetReffuncResult( void** ppResult, CAssoc* const& pAssoc )
{
	CAssoc::Release( g_pAssocResult );
	g_pAssocResult = pAssoc;
	CAssoc::AddRef( g_pAssocResult );

	*ppResult = const_cast<CAssoc**>( &pAssoc );
	return g_vtAssoc;
}

//#########################################################
//        命令
//#########################################################
//------------------------------------------------
// 構築 (dim)
//------------------------------------------------
void AssocNew()
{
	PVal* const pval = code_getpval();
	int len[4];
	for ( int i = 0; i < 4; ++ i ) {
		len[i] = code_getdi(0);
	}

	exinfo->HspFunc_dim( pval, g_vtAssoc, 0, len[0], len[1], len[2], len[3] );
	return;
}

//------------------------------------------------
// 破棄
//------------------------------------------------
void AssocDelete()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != g_vtAssoc ) puterror( HSPERR_TYPE_MISMATCH );

	CAssoc*& self = *VtAssoc::getPtr(pval);
	if ( self ) {
		self->Release();
		self = nullptr;
	}
	return;
}

//------------------------------------------------
// 外部変数のインポート
//------------------------------------------------
static void AssocImportImpl( CAssoc* self, char const* src );

void AssocImport()
{
	CAssoc* const self = code_get_assoc();
	if ( !self ) puterror( HSPERR_ILLEGAL_FUNCTION );

	while ( code_isNextArg() ) {
		char const* const src = code_gets();
		AssocImportImpl( self, src );
	}
	return;
}

static void AssocImportImpl( CAssoc* const self, char const* const src )
{
	// モジュール名
	if ( src[0] == '@' ) {
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
		/*
		bool bGlobal = ( src[1] == '\0' );

		// すべての静的変数から絞り込み検索
		for ( int i = 0; i < ctx->hsphed->max_val; ++ i ) {
			char const* const name   = exinfo->HspFunc_varname( i );
			char const* const nameAt = strchr(name, '@');

			if (   ( bGlobal && nameAt == NULL )			// グローバル変数
				|| (!bGlobal && nameAt != NULL && !strcmp(nameAt + 1, src + 1) )	// モジュール内変数 (モジュール名一致)
			) {
				self->Insert( name, &ctx->mem_var[i] );
			}
		}
		//*/

	// 変数名
	} else {
		PVal* const pval = hpimod::seekSttVar(src);
		if ( pval ) {
			self->Insert( src, pval );
		}
	}

	return;
}

//------------------------------------------------
// キーを挿入・除去する
// 
// @ キーは一つの引数として受け取る。
// @ 挿入はほぼ無意味。
//------------------------------------------------
void AssocInsert()
{
	CAssocHolder self = code_get_assoc();
	char const* const key = code_gets();

	if ( !self ) puterror( HSPERR_ILLEGAL_FUNCTION );

	// 参照された内部変数
	PVal* const pvInner = self->At( key );

	// 初期値 (省略可能)
	if ( code_getprm() > PARAM_END ) {
		int const fUserElem = pvInner->support & CAssoc::HSPVAR_SUPPORT_USER_ELEM;

		PVal_assign( pvInner, mpval->pt, mpval->flag );

		pvInner->support |= fUserElem;
	}

	return;
}

void AssocRemove()
{
	CAssocHolder self = code_get_assoc();
	char const* const key = code_gets();

	if ( !self ) puterror( HSPERR_ILLEGAL_FUNCTION );

	self->Remove( key );
	return;
}

//------------------------------------------------
// 内部変数の dim
// 
// @prm: [ assoc("key"), vartype, len1..4 ]
//------------------------------------------------
void AssocDim()
{
	PVal* const pvInner = code_get_assoc_pval();
	if ( !pvInner ) puterror( HSPERR_ILLEGAL_FUNCTION );

	int const fUserElem = pvInner->support & CAssoc::HSPVAR_SUPPORT_USER_ELEM;

	int const vflag = code_getdi( pvInner->flag );	// 型タイプ値
	int len[4];
	for ( int i = 0; i < hpimod::ArrayDimMax; ++ i ) {
		len[i] = code_getdi(0);		// 要素数
	}

	// 配列として初期化する
	exinfo->HspFunc_dim( pvInner, vflag, 0, len[0], len[1], len[2], len[3] );

	pvInner->support |= fUserElem;
	return;
}

//------------------------------------------------
// 内部変数のクローンを作る
//------------------------------------------------
void AssocClone()
{
	PVal* const pvInner = code_get_assoc_pval();	// クローン元
	PVal* const pval    = code_getpval();			// クローン先

	if ( !pvInner || !pval ) puterror( HSPERR_ILLEGAL_FUNCTION );

	PVal_cloneVar( pval, pvInner );
	return;
}

//------------------------------------------------
// assoc 連結 | 複製
//------------------------------------------------
static void AssocChainOrCopy( bool bCopy )
{
	CAssocHolder dst = code_get_assoc();
	CAssoc* const src = code_get_assoc();
	if ( !dst || !src ) puterror( HSPERR_ILLEGAL_FUNCTION );

	if ( bCopy ) {
		dst->Copy( src );
	} else {
		dst->Chain( src );
	}
	return;
}

void AssocCopy()  { AssocChainOrCopy( true  ); }
void AssocChain() { AssocChainOrCopy( false ); }

//------------------------------------------------
// assoc 消去
//------------------------------------------------
void AssocClear()
{
	CAssoc* const self = code_get_assoc();
	if ( !self ) puterror( HSPERR_ILLEGAL_FUNCTION );

	self->Clear();
	return;
}

//#########################################################
//        関数
//#########################################################
//------------------------------------------------
// 構築 (一時変数)
//------------------------------------------------
int AssocNewTemp(void** ppResult)
{
	return SetReffuncResult( ppResult, CAssoc::New() );	// 直後に mpval に所有される
}

//------------------------------------------------
// 構築 (複製)
//------------------------------------------------
int AssocNewTempDup(void** ppResult)
{
	CAssoc* const src = code_get_assoc();
	if ( !src ) puterror( HSPERR_ILLEGAL_FUNCTION );

	CAssoc* const newOne = CAssoc::New();
	newOne->Chain( src );
	return SetReffuncResult( ppResult, newOne );	// 直後に mpval に所有される
}

//------------------------------------------------
// null か
//------------------------------------------------
int AssocIsNull(void** ppResult)
{
	CAssoc* const self = code_get_assoc();
	return SetReffuncResult( ppResult, HspBool(self != nullptr));
}

//------------------------------------------------
// 内部変数の情報を得る
//------------------------------------------------
int AssocVarinfo(void** ppResult)
{
	PVal* const pvInner = code_get_assoc_pval();
	if ( !pvInner ) puterror( HSPERR_ILLEGAL_FUNCTION );

	int const varinfo = code_getdi( VARINFO_NONE );
	int const opt     = code_getdi( 0 );

	switch ( varinfo ) {
		case VARINFO_FLAG:   return SetReffuncResult( ppResult, (int)pvInner->flag );
		case VARINFO_MODE:   return SetReffuncResult( ppResult, (int)pvInner->mode );
		case VARINFO_LEN:    return SetReffuncResult( ppResult, pvInner->len[opt + 1] );
		case VARINFO_SIZE:   return SetReffuncResult( ppResult, pvInner->size );
		case VARINFO_PT:     return SetReffuncResult( ppResult, (int)(getHvp(pvInner->flag))->GetPtr(pvInner) );
		case VARINFO_MASTER: return SetReffuncResult( ppResult, (int)pvInner->master );
		default:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );
			throw;
	}
}

//------------------------------------------------
// 要素数
//------------------------------------------------
int AssocSize(void** ppResult)
{
	CAssoc* const self = code_get_assoc();
	int const size = (self ? self->Size() : 0);
	return SetReffuncResult(ppResult, size);
}

//------------------------------------------------
// 指定キーが存在するか
//------------------------------------------------
int AssocExists(void** ppResult)
{
	CAssocHolder self = code_get_assoc();
	char const* const key  = code_gets();

	return SetReffuncResult( ppResult, (int)(self ? self->Exists(key) : false) );
}

//------------------------------------------------
// AssocForeach 更新処理
//------------------------------------------------
int AssocForeachNext(void** ppResult)
{
	CAssocHolder self = code_get_assoc();	// assoc
	PVal* const pval = code_get_var();		// iter (key)
	int const idx  = code_geti();

	bool bContinue =			// 続けるか否か
		( !!self && idx >= 0
		&& static_cast<size_t>(idx) < self->Size()
		);

	if ( bContinue ) {
		auto iter = self->begin();

		for ( int i = 0; i < idx; ++ i )	// 要素 [idx] への反復子を取得する
			++ iter;

		// キーの文字列を代入する
		code_setva( pval, pval->offset, HSPVAR_FLAG_STR, iter->first.c_str() );
	}

	return SetReffuncResult( ppResult, HspBool(bContinue) );
}

//------------------------------------------------
// assoc 式
//------------------------------------------------
static int const AssocResultExprMagicNumber = 0xA550C;

int AssocResult( void** ppResult )
{
	if ( g_pAssocResult ) {			// 前のを解放する
		CAssoc::Release( g_pAssocResult );
		g_pAssocResult = nullptr;
	}

	g_pAssocResult = code_get_assoc();
	CAssoc::AddRef( g_pAssocResult );

	return SetReffuncResult(ppResult, AssocResultExprMagicNumber);
}

int AssocExpr( void** ppResult )
{
	// AssocResult が呼ばれるはず
	if ( code_geti() != AssocResultExprMagicNumber ) puterror(HSPERR_ILLEGAL_FUNCTION);

	*ppResult = &g_pAssocResult;
	return g_vtAssoc;
}

//------------------------------------------------
// 終了時関数
//------------------------------------------------
void AssocTerm()
{
	CAssoc::Release( g_pAssocResult ); g_pAssocResult = nullptr;
	return;
}
