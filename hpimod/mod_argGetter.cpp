// 引数取得モジュール

#include "hsp3plugin_custom.h"
#include "mod_argGetter.h"
#include "mod_makepval.h"

namespace hpimod
{

//##########################################################
//    引数の取得
//##########################################################
//------------------------------------------------
// 配列要素を取得する
//------------------------------------------------
PVal* code_get_var()
{
	PVal* pval;
	code_getva(&pval);
	return pval;
}

//------------------------------------------------
// 文字列を取得する (hspmalloc で確保する)
// 
// @ 解放義務(hspfree)は呼び出し元にある。
//------------------------------------------------
size_t code_getds_ex(char** ppStr, char const* defstr)
{
	char* const pStr = code_getds(defstr);
	size_t const len = std::strlen(pStr);
	size_t const size = (len + 1) * sizeof(char);

	*ppStr = reinterpret_cast<char*>(hspmalloc(size));
	strncpy_s( *ppStr, size, pStr, len );
	(*ppStr)[len] = '\0';		// 終端
	return len;
}

#if 0
//------------------------------------------------
// 文字列か数値を取得する
// 
// @ 文字列なら sbAlloc で確保、文字列をコピーする。
// @result = int
//		*ppStr が nullptr なら、返値が有効。
//		そうでない場合、*ppStr が有効。
//------------------------------------------------
int code_get_int_or_str( char** ppStr )
{
	*ppStr = nullptr;

	if ( code_getprm() <= PARAM_END ) return 0;

	switch ( mpval->flag ) {
		case HSPVAR_FLAG_INT:
			return *(int*)( mpval->pt );

		case HSPVAR_FLAG_DOUBLE:
			return static_cast<int>( *(double*)(mpval->pt) );

		case HSPVAR_FLAG_STR:
		{
			*ppStr = hspmalloc( getHvp(mpval->flag)->GetSize((PDAT*)mpval->pt) /*PVal_size(mpval)*/ + 1 );
			strcpy( *ppStr, (char*)(mpval->pt) );
			return 0;
		}
		default:
			puterror( HSPERR_TYPE_MISMATCH );
	}
	return 0;
}
#endif

//------------------------------------------------
// 型タイプ値を取得する
// 
// @ 文字列 or 数値
// @error 文字列で非型名        => HSPERR_ILLEGAL_FUNCTION
// @error 文字列でも数値でもない => HSPERR_TYPE_MISMATCH
//------------------------------------------------
int code_get_vartype( int deftype )
{
	int const prm = code_getprm();
	if ( prm == PARAM_DEFAULT ) return deftype;
	if ( prm <= PARAM_END ) return HSPVAR_FLAG_NONE;

	switch ( mpval->flag ) {
		// 型タイプ値
		case HSPVAR_FLAG_INT:
			return VtTraits::derefValptr<vtInt>(mpval->pt);

		// 型名
		case HSPVAR_FLAG_STR:
		{
			auto const vp = seekHvp( VtTraits::asValptr<vtStr>(mpval->pt) );
			if ( !vp ) puterror( HSPERR_ILLEGAL_FUNCTION );

			return vp->flag;
		}
		default:
			puterror( HSPERR_TYPE_MISMATCH );
	}
}

//------------------------------------------------
// 実行ポインタを取得する( ラベル、省略可能 )
//------------------------------------------------
label_t code_getdlb( label_t defLabel )
{
	try {
		return code_getlb();

	} catch( HSPERROR err ) {
		if ( err == HSPERR_LABEL_REQUIRED ) {
			return defLabel;
		}

		puterror( err );
	}

	/*
	label_t lb = nullptr;

	// リテラル( *lb )の場合
	// @ *val にはラベルID ( ctx->mem_ot の要素番号 )が入っている。
	// @ code_getlb() で得られるのはラベルが指す実行ポインタなるもの。
	if ( *type == TYPE_LABEL ) {	// ラベル定数
		lb = code_getlb();

	// ラベル型変数の場合
	// @ code_getlb() と同じ処理, mpval を更新する。
	// @ 条件 ( *type == TYPE_VAR ) では、ラベル型を返す関数やシステム変数に対応できない。
	} else {
		// ラベルの指す実行ポインタを取得
		if ( code_getprm() <= PARAM_END )         return nullptr;
		if ( mpval->flag   != HSPVAR_FLAG_LABEL ) return nullptr;

		// ラベルのポインタを取得する( 変数の実体から取り出す )
		lb = *(label_t*)( mpval->pt );
	}

	return lb;
	//*/
}

//------------------------------------------------
// ラベル実行ポインタを取得する
// @ ???
//------------------------------------------------
//pExec_t code_getlb2(void)
//{
//	pExec_t pLbExec = code_getlb();
//	code_next();
//	*exinfo->npexflg &= ~EXFLG_2;
//	return pLbExec;
//}

//------------------------------------------------
// インスタンスを取得する
//------------------------------------------------
FlexValue* code_get_struct()
{
	PVal* const pval = code_get_var();
	if ( pval->flag != HSPVAR_FLAG_STRUCT ) puterror(HSPERR_TYPE_MISMATCH);

	return VtTraits::asValptr<vtStruct>(getHvp(HSPVAR_FLAG_STRUCT)->GetPtr(pval));
}

//------------------------------------------------
// モジュールクラス識別子を取得
//------------------------------------------------
stdat_t code_get_modcls()
{
	stprm_t const stprm = code_get_stprm();
	if ( stprm->mptype != MPTYPE_STRUCTTAG ) puterror( HSPERR_STRUCT_REQUIRED );
	return STRUCTPRM_getStDat(stprm);
}

//------------------------------------------------
// 構造体パラメータを取得
//------------------------------------------------
stprm_t code_get_stprm()
{
	if ( *type != TYPE_STRUCT ) puterror( HSPERR_STRUCT_REQUIRED );

	stprm_t const pStPrm = getSTRUCTPRM(*val);
	code_get_singleToken();
	return pStPrm;
}

//------------------------------------------------
// 1つの字句からなる式を受け取る
//
// @ その字句の値自体は先読み (type, val) で既に取得できている。
//------------------------------------------------
int code_get_singleToken()
{
	int const chk = code_get_procHeader();
	if ( chk <= PARAM_END ) return chk;

//	int const type_bak = *type, val_bak = *val;
	code_next();

	// 次が文頭や式頭ではなく、')' でもない → 与えられた引数式が2字句以上でできている
	if ( *exinfo->npexflg & (EXFLG_1 | EXFLG_2) || (*type == TYPE_MARK && *val == ')') ) {
		*exinfo->npexflg &= ~EXFLG_2;
		return (*type == TYPE_MARK && *val == ')')
			? PARAM_SPLIT
			: PARAM_OK;
	} else {
		puterror(HSPERR_SYNTAX);
	}
}

//##########################################################
//    配列添字の解決
//##########################################################
//------------------------------------------------
// 添字の取り出し (通常配列)
// 
// @ '(' を取り出した直後の状態で呼ぶ
//------------------------------------------------
void code_expand_index_int( PVal* pval, bool bRhs )
{
	HspVarCoreReset(pval);	// 配列添字の情報を初期化する

	int n = 0;
	PVal tmpPVal;

	for (;;) {
		// 添字の状態を保存
		HspVarCoreCopyArrayInfo( &tmpPVal, pval );

		int const prm = code_getprm();

		// エラーチェック
		if ( prm == PARAM_DEFAULT ) {
			n = 0;

		} else if ( prm <= PARAM_END ) {
			puterror( HSPERR_BAD_ARRAY_EXPRESSION );

		} else if ( mpval->flag != HSPVAR_FLAG_INT ) {
			puterror( HSPERR_TYPE_MISMATCH );
		}

		// 添字の状態を戻す
		HspVarCoreCopyArrayInfo( pval, &tmpPVal );

		if ( prm != PARAM_DEFAULT ) {
			n = VtTraits::derefValptr<vtInt>(mpval->pt);
		}

		code_index_int( pval, n, bRhs );	// 配列要素指定 (int)
		if ( prm == PARAM_SPLIT ) break;
	}
	return;
}

//------------------------------------------------
// 添字括弧の取り出し (通常配列)
//------------------------------------------------
static void code_checkarray( PVal* pval, bool bRhs )
{
	if ( *type == TYPE_MARK && *val == '(' ) {
		code_next();

		code_expand_index_int(pval, bRhs);

		if ( !(*type == TYPE_MARK && *val == ')') ) {
			puterror( HSPERR_BAD_ARRAY_EXPRESSION );
		}
		code_next();

	// 添字がなければ初期化だけする
	} else {
		HspVarCoreReset( pval );
	}
	return;
}

void code_checkarray2( PVal* pval ) { code_expand_index_int( pval, false ); }
void code_checkarray1( PVal* pval ) { code_expand_index_int( pval, true  ); }

//------------------------------------------------
// 添字括弧の取り出し (連想配列, 左)
//------------------------------------------------
void code_checkarray_obj2( PVal* pval )
{
	HspVarCoreReset( pval );

	if ( *type == TYPE_MARK && *val == '(' ) {
		code_next();

		getHvp(pval->flag)->ArrayObject( pval );	// 添字参照

		if ( !(*type == TYPE_MARK && *val == ')') ) {
			puterror( HSPERR_BAD_ARRAY_EXPRESSION );
		}
		code_next();
	}
	return;
}

//------------------------------------------------
// 添字括弧の取り出し (連想配列, 右)
// 
// @prm pval   : 添字指定される配列変数
// @prm mptype : 汎用データの型タイプ値を返す
// @result     : 汎用データへのポインタ
//------------------------------------------------
PDAT* code_checkarray_obj1( PVal* pval, int& mptype )
{
	HspVarCoreReset( pval );

	if ( *type == TYPE_MARK && *val == '(' ) {
		code_next();

		PDAT* const pResult = getHvp( pval->flag )->ArrayObjectRead( pval, &mptype );

		if ( !(*type == TYPE_MARK && *val == ')') ) {
			puterror( HSPERR_BAD_ARRAY_EXPRESSION );
		}
		code_next();
		return pResult;
	}

	mptype = pval->flag;
	return PVal_getptr(pval);
}

//------------------------------------------------
// 添字の取り出し (通常/連想, 中身だけ)
// 
// @ '(' を取り出した直後に呼ばれる。
//------------------------------------------------
void code_expand_index_lhs( PVal* pval )
{
	// 連想配列型 => ArrayObject() を呼ぶ
	if ( pval->support & HSPVAR_SUPPORT_ARRAYOBJ ) {
		getHvp( pval->flag )->ArrayObject( pval );

	// 通常配列型 => 次元の数だけ要素を取り出す
	} else {
		PVal pvalTemp;
		HspVarCoreReset( pval );

		for ( int i = 0; i < ArrayDimMax && !(*type == TYPE_MARK && *val == ')'); ++ i ) {
			HspVarCoreCopyArrayInfo( &pvalTemp, pval );
			int const idx = code_geti();
			HspVarCoreCopyArrayInfo( pval, &pvalTemp );

			code_index_int_lhs( pval, idx );
		}
	}

	return;
}

PDAT* code_expand_index_rhs( PVal* pval, int& mptype )
{
	// 連想配列型 => ArrayObjectRead() を呼ぶ
	if ( pval->support & HSPVAR_SUPPORT_ARRAYOBJ ) {
		return getHvp( pval->flag )->ArrayObjectRead( pval, &mptype );

	// 通常配列型 => 次元の数だけ要素を取り出す
	} else {
		PVal pvalTemp;
		HspVarCoreReset( pval );

		for ( int i = 0; i < ArrayDimMax && !(*type == TYPE_MARK && *val == ')'); ++ i ) {
			HspVarCoreCopyArrayInfo( &pvalTemp, pval );
			int const idx = code_geti();
			HspVarCoreCopyArrayInfo( pval, &pvalTemp );

			code_index_int_rhs( pval, idx );
		}

		mptype = pval->flag;
		return getHvp( pval->flag )->GetPtr( pval );
	}
}

//------------------------------------------------
// 配列要素の設定 (通常配列, 1つだけ, 左右)
// 
// @ 通常型 (int) のみ。
// @ Reset 後に次元数だけ連続で呼ばれる。
//------------------------------------------------
void code_index_int( PVal* pval, int offset, bool bRhs )
{
	if ( !bRhs ) {
		code_index_int_lhs( pval, offset );		// 自動拡張する
	} else {
		code_index_int_rhs( pval, offset );		// 自動拡張しない
	}
	return;
}

// 左辺値として参照
void code_index_int_lhs( PVal* pval, int offset )
{
	if ( pval->arraycnt >= 5 ) puterror( HSPVAR_ERROR_ARRAYOVER );
	if ( pval->arraycnt == 0 ) {
		pval->arraymul = 1;		// 倍率初期値
	} else {
		pval->arraymul *= pval->len[pval->arraycnt];
	}
	++pval->arraycnt;
	if ( offset < 0 ) puterror( HSPVAR_ERROR_ARRAYOVER );
	if ( offset >= pval->len[pval->arraycnt] ) {							// 配列拡張が必要
		if ( (pval->arraycnt >= 4 || pval->len[pval->arraycnt + 1] == 0)	// 配列拡張が可能
			&& (pval->support & HSPVAR_SUPPORT_FLEXARRAY)					// 可変長配列サポート => 配列を拡張する
		) {
			exinfo->HspFunc_redim( pval, pval->arraycnt, offset + 1 );
			pval->offset += offset * pval->arraymul;
			return;
		}
		puterror( HSPVAR_ERROR_ARRAYOVER );
	}
	pval->offset += offset * pval->arraymul;
	return;
}

// 右辺値として参照
extern void code_index_int_rhs( PVal* pval, int offset )
{
	exinfo->HspFunc_array( pval, offset );
#if 0
	if ( pval->arraycnt >= 5 ) puterror(HSPVAR_ERROR_ARRAYOVER);
	if ( pval->arraycnt == 0 ) {
		pval->arraymul = 1;
	} else {
		pval->arraymul *= pval->len[pval->arraycnt];
	}
	++pval->arraycnt;
	if ( offset < 0 ) puterror(HSPVAR_ERROR_ARRAYOVER);
	if ( offset >= (pval->len[pval->arraycnt]) ) {
		puterror(HSPVAR_ERROR_ARRAYOVER);
	}
	pval->offset += offset * pval->arraymul;
#endif
	return;
}

// 添字の初期化
void code_index_reset(PVal* pval)
{
	// 標準配列の場合のみ初期化する
	if ( PVal_supportArray(pval) && !(pval->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {	
		HspVarCoreReset(pval);
	}
	return;
}

//------------------------------------------------
// 
//------------------------------------------------

//##########################################################
//    代入処理エミュレート
//##########################################################

//------------------------------------------------
// 連続代入 (通常配列)
// 
// @ 1つ目の代入は終了しているとする
// @ 代入する値がない => do nothing
//------------------------------------------------
void code_assign_multi( PVal* pval )
{
	if ( !code_isNextArg() ) return;

	int const len1 = pval->len[1];
	assert(len1 > 0);

	// aptr = 一次元目の添字 + baseaptr が成立
	APTR baseaptr = pval->offset % len1;
	APTR aptr = pval->offset - baseaptr;

	do {
		int const prm = code_getprm();				// 次に代入する値を取得
		if ( prm <= PARAM_END ) puterror( HSPERR_SYNTAX );
	//	if ( !(pval->support & HSPVAR_SUPPORT_ARRAYOBJ) && pval->flag != mpval->flag ) {
	//		puterror( HSPERR_INVALID_ARRAYSTORE );	// 型変更はできない
	//	}

		baseaptr ++;

		pval->arraycnt = 0;							// 配列指定カウンタをリセット
		pval->offset   = aptr;
		code_index_int_lhs( pval, baseaptr );		// 配列チェック

		// 代入
		PVal_assign( pval, mpval->pt, mpval->flag );
	} while ( code_isNextArg() );

	return;
}

//##########################################################
//    その他
//##########################################################
//------------------------------------------------
// 引数が続くかどうか
// 
// @ 命令形式、関数形式どちらでもＯＫ
//------------------------------------------------
bool code_isNextArg()
{
	return !( *exinfo->npexflg & EXFLG_1 || ( *type == TYPE_MARK && *val == ')' ) );
}

//------------------------------------------------
// code_get の冒頭の処理
//------------------------------------------------
int code_get_procHeader()
{
	int& exflg = *exinfo->npexflg;

	// 終了, or 省略
	if ( exflg & EXFLG_1 ) return PARAM_END;	// 文頭、すなわちパラメーター終端
	if ( exflg & EXFLG_2 ) {					// パラメーター区切り(デフォルト時)
		exflg &= ~EXFLG_2;
		return PARAM_DEFAULT;
	}

	if ( *type == TYPE_MARK ) {
		// パラメーター省略時('?')
		if ( *val == 63 ) {
			code_next();
			exflg &= ~EXFLG_2;
			return PARAM_DEFAULT;

			// 関数内のパラメーター省略時
		} else if ( *val == ')' ) {
			exflg &= ~EXFLG_2;
			return PARAM_ENDSPLIT;
		}
	}

	// 式の本体を取り出す
	return PARAM_OK;
}

//------------------------------------------------
// 次の引数を読み飛ばす
// 
// @ exflg をなんとかしようとしてみる。
// @ result : PARAM_* (code_getprm と同じ)
//------------------------------------------------
int code_skipprm()
{
	{
		int const chk = code_get_procHeader();
		if ( chk <= PARAM_END ) return chk;
	}
	int& exflg = *exinfo->npexflg;

	// 引数の式の読み飛ばし処理
	for ( int lvBracket = 0; ; ) {			// 無限ループ
		if ( *type == TYPE_MARK ) {
			if ( *val == '(' ) lvBracket ++;
			if ( *val == ')' ) lvBracket --;
		}
		code_next();

		if ( lvBracket == 0 && (exflg & (EXFLG_1 | EXFLG_2) || (*type == TYPE_MARK && *val == ')')) ) {
			// 括弧の中ではなく、かつ式に後続の字句が見えたら終了
			break;
		}
	}

	if ( exflg ) exflg &= ~EXFLG_2;

	// 終了
	return ( *type == TYPE_MARK && *val == ')' )
		? PARAM_SPLIT
		: PARAM_OK;
}

//------------------------------------------------
// 次の特定のコードを無視する
//------------------------------------------------
bool code_next_expect( int expect_type, int expect_val )
{
	if ( !(*type == expect_type && *val == expect_val) ) return false;
	code_next();
	return true;
}

} // namespace hpimod
