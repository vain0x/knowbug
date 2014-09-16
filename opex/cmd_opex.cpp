// opex - command

#include "mod_makepval.h"
#include "mod_argGetter.h"
#include "mod_func_result.h"

#include "cmd_opex.h"

using namespace hpimod;

static operator_t GetOpFuncPtr( HspVarProc* hvp, OPTYPE optype );

//------------------------------------------------
// 代入演算
// 
// @ 連続代入演算対応
//------------------------------------------------
static PVal* assign_impl()
{
	PVal* const pval = code_get_var();	// 代入先

	// 第一値の代入
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	PVal_assign( pval, mpval->pt, mpval->flag );

	// 連続代入
	code_assign_multi( pval );

	return pval;
}

void assign()
{
	assign_impl();
	return;
}

int assign( void** ppResult )
{
	PVal* const pval = assign_impl();

	*ppResult = PVal_getptr(pval);
	return pval->flag;
}

//------------------------------------------------
// 交換演算 (左ロテート)
//------------------------------------------------
static PVal* swap_impl()
{
	static size_t const stc_maxCnt = 16;

	PVal* pval[stc_maxCnt];
	APTR  aptr[stc_maxCnt];

	// 引数は最低2つ必要
	aptr[0] = code_getva( &pval[0] );
	aptr[1] = code_getva( &pval[1] );

	int cnt = 2;
	for ( ; cnt < stc_maxCnt && code_isNextArg(); ++cnt ) {
		aptr[cnt] = code_getva( &pval[cnt] );
	}

	// 交換
	for ( int i = cnt - 1; i > 0; -- i ) {
		PVal_swap( pval[i], pval[(i + 1) % cnt], aptr[i], aptr[(i + 1) % cnt] );
	}

	return pval[cnt - 1];	// 最後を返却する
}

void swap()
{
	swap_impl();
	return;
}

int swap( void** ppResult )
{
	PVal* const pval = swap_impl();
	*ppResult = PVal_getptr( pval );
	return pval->flag;
}

//------------------------------------------------
// 弱参照演算
//------------------------------------------------
static PVal* clone_impl()
{
	PVal* const pvDst = code_get_var();
	PVal* const pvSrc = code_get_var();

	PVal_cloneVar( pvDst, pvSrc );
	return pvDst;
}

void clone()
{
	clone_impl();
	return;
}

int clone( void** ppResult )
{
	PVal* const pval = clone_impl();
	*ppResult = PVal_getptr( pval );
	return pval->flag;
}

//------------------------------------------------
// キャスト演算
// 
// @ mpval の関係で、先に flag を受け取るが、
// @	マクロで castTo( value, flagDst ) の順に調整する。
//------------------------------------------------
int castTo( void** ppResult )
{
	vartype_t const flag = code_geti();
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );

	*ppResult = const_cast<PDAT*>(
		Valptr_cnvTo( reinterpret_cast<PDAT*>(mpval->pt), mpval->flag, flag )
	);
	return flag;
}

//------------------------------------------------
// メンバ変数の取り出し
//------------------------------------------------
static PVal* GetMemberOf(void* prmstack, stprm_t stprm)
{
	assert(stprm->mptype == MPTYPE_LOCALVAR);

	auto const prmstk = reinterpret_cast<char*>(prmstack);
	return reinterpret_cast<PVal*>(prmstk + stprm->offset);
}

static PVal* code_get_struct_member()
{
	FlexValue* const modSrc = code_get_struct();
	stprm_t const stprm = code_get_stprm();

	if ( !(modSrc->type != FLEXVAL_TYPE_NONE
		&& FlexValue_getModuleTag(modSrc)->subid == stprm->subid
		&& modSrc->ptr) ) puterror(HSPERR_INVALID_STRUCT_SOURCE);

	PVal* const pvMember = GetMemberOf(modSrc->ptr, stprm);

	// この後メンバ変数の添字処理を行う
	return pvMember;
}

static PVal* code_get_struct_member_lhs()
{
	PVal* const pvMember = code_get_struct_member();

	if ( *type == TYPE_MARK && *val == '(' ) {
		code_next();
		code_expand_index_lhs(pvMember);
		code_next_expect(TYPE_MARK, ')');

	} else if ( PVal_supportArray(pvMember) && !(pvMember->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {
		HspVarCoreReset(pvMember);
	}
	return pvMember;
}

static void* code_get_struct_member_rhs(int& vtype)
{
	PVal* const pvMember = code_get_struct_member();

	if ( *type == TYPE_MARK && *val == '(' ) {
		int mptype = 0;

		code_next();
		void* pResult = code_expand_index_rhs(pvMember, &vtype);
		code_next_expect(TYPE_MARK, ')');

		return pResult;

	} else if ( PVal_supportArray(pvMember) && !(pvMember->support & HSPVAR_SUPPORT_ARRAYOBJ) ) {
		HspVarCoreReset(pvMember);
	}

	vtype = pvMember->flag;
	return pvMember->pt;
}

// @prm: [ mod, member,  src ]
// memberOf(mod, member) = src と代入する。
void memberOf()
{
	PVal* const pvMember = code_get_struct_member_lhs();
	int const chk = code_getprm();
	if ( chk <= PARAM_END ) puterror(HSPERR_NO_DEFAULT);

	PVal_assign(pvMember, mpval->pt, mpval->flag);
	return;
}

// @prm: [ src_mod, src_member ]
int memberOf(void** ppResult)
{
	int vtype = HSPVAR_FLAG_NONE;
	*ppResult = code_get_struct_member_rhs(vtype);
	return vtype;
}

// @prm: [ dst_var, src_mod, src_member ]
void memberClone()
{
	PVal* const pvDst = code_get_var();
	PVal* const pvMember = code_get_struct_member_lhs();
	PVal_cloneVar(pvDst, pvMember);
	return;
}

//------------------------------------------------
// 
//------------------------------------------------

//#########################################################
//        関数
//#########################################################
//------------------------------------------------
// 短絡論理演算
//------------------------------------------------
int shortLogOp( void** ppResult, bool bAnd )
{
	bool bResult = (bAnd ? true : false);

	for(;;) {
		// 条件
		int const prm = code_getprm();
		if ( prm <= PARAM_END ) break;
		if ( mpval->flag != HSPVAR_FLAG_INT ) puterror( HSPERR_TYPE_MISMATCH );

		bool const predicate = (*(int*)mpval->pt != 0);

		// and: 1つでも false があれば false
		if ( bAnd && !predicate ) {
			bResult = false;
			break;

		// or: 1つでも true があれば true
		} else if ( !bAnd && predicate ) {
			bResult = true;
			break;
		}
	}

	// 残りの引数を捨てる
	while ( code_skipprm() > PARAM_END )
		;

	return SetReffuncResult( ppResult, bResult );
}

//------------------------------------------------
// 比較演算
//------------------------------------------------
int cmpLogOp( void** ppResult, bool bAnd )
{
	bool bResult = (bAnd ? true : false);

	// 比較元の値を取得
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );

	vartype_t const flag = mpval->flag;
	auto const pfOp = GetOpFuncPtr( getHvp(flag), (OPTYPE)OPTYPE_EQ );

	if ( pfOp == NULL ) puterror( HSPERR_TYPE_MISMATCH );

	// 比較元の値を保存
	size_t const size = PVal_size( mpval );
	void* const pTarget    = hspmalloc(size);	// 元データ
	void* const pTargetTmp = hspmalloc(size);	// 作業用バッファ
	if ( !pTarget || !pTargetTmp ) puterror( HSPERR_OUT_OF_MEMORY );

	std::memcpy( pTarget, mpval->pt, size );			// 丸々コピーしておく

	// 比較値と比較
	for(;;) {
		int prm = code_getprm();
		if ( prm <= PARAM_END ) break;

		bool bPred;

		if ( mpval->flag != flag ) {
			bPred = false;	// 型が違う時点でアウト

		} else {
			// pTarget を作業用バッファコピー
			memcpy( pTargetTmp, pTarget, size );

			// 演算
			(*pfOp)( (PDAT*)pTargetTmp, mpval->pt );	// 演算子の動作
			bPred = ( *(int*)pTargetTmp != 0 );
		}

		if ( bAnd && !bPred ) {
			bResult = false;
			break;

		} else if ( !bAnd && bPred ) {
			bResult = true;
			break;
		}
	}

	// 残りの引数を捨てる
	while ( code_skipprm() > PARAM_END )
		;

	// バッファを解放
	if ( pTarget    ) hspfree( pTarget    );
	if ( pTargetTmp ) hspfree( pTargetTmp );

	return SetReffuncResult( ppResult, bResult );
}

//------------------------------------------------
// 条件演算 (?:)
//------------------------------------------------
int which(void** ppResult)
{
	bool const predicate = (code_geti() != 0);
	if ( !predicate ) code_skipprm();		// 偽 => 真の部分を飛ばす

	if ( code_getprm() <= PARAM_END ) {
		puterror( HSPERR_NO_DEFAULT );
	}

	// 返値となる値を取り出す
	vartype_t const flag = mpval->flag;
	*ppResult = mpval->pt;

	if ( predicate ) code_skipprm();		// 真 => 偽の部分を飛ばす
	return flag;
}

//------------------------------------------------
// 分岐関数
//------------------------------------------------
int what(void** ppResult)
{
	int const idx = code_geti();
	if ( idx < 0 ) puterror( HSPERR_ILLEGAL_FUNCTION );

	// 前の部分を飛ばしていく
	for( int i = 0; i < idx; i ++ ) {
		if ( code_skipprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	}

	// 返値となる値を取り出す
	if ( code_getprm() <= PARAM_END ) puterror( HSPERR_NO_DEFAULT );
	vartype_t const flag = mpval->flag;
	*ppResult = mpval->pt;

	// 残りの引数を捨てる
	while ( code_skipprm() > PARAM_END )
		;

	return flag;
}

//------------------------------------------------
// リスト式
// 
// @ 各引数を評価して、最後の引数の値を返す。
//------------------------------------------------
int exprs( void** ppResult )
{
	while ( code_getprm() > PARAM_END )
		;

	*ppResult = mpval->pt;
	return mpval->flag;
}

//------------------------------------------------
// 
//------------------------------------------------

//#########################################################
//        システム変数
//#########################################################
//------------------------------------------------
// (kw) 定数ポインタ
// 
// @ kw_constptr || CONST_VALUE
//------------------------------------------------
int kw_constptr( void** ppResult )
{
	if ( *exinfo->npexflg & (EXFLG_1 | EXFLG_2) ) puterror( HSPERR_SYNTAX );

	int result;

	switch ( *type ) {
		case TYPE_STRING:
		case TYPE_DNUM:
			result = reinterpret_cast<int>( &ctx->mem_mds[*val] );
			break;

		case TYPE_INUM:
			puterror( HSPERR_UNSUPPORTED_FUNCTION );	// CS中に埋め込まれているのでとれない
		default:
			puterror( HSPERR_SYNTAX );
	}
	code_next();

	if ( !(*type == TYPE_MARK && *val == OPTYPE_OR) ) puterror( HSPERR_SYNTAX );
	code_next();

	return SetReffuncResult( ppResult, result, HSPVAR_FLAG_INT );
}

//------------------------------------------------
// 
//------------------------------------------------

//#########################################################
//        下請け
//#########################################################
//------------------------------------------------
// Hvp から演算処理関数を取り出す
//------------------------------------------------
operator_t GetOpFuncPtr( HspVarProc* hvp, OPTYPE optype )
{
	if ( !hvp ) return nullptr;
	switch ( optype ) {
		case OPTYPE_ADD: return hvp->AddI;
		case OPTYPE_SUB: return hvp->SubI;
		case OPTYPE_MUL: return hvp->MulI;
		case OPTYPE_DIV: return hvp->DivI;
		case OPTYPE_MOD: return hvp->ModI;
		case OPTYPE_AND: return hvp->AndI;
		case OPTYPE_OR:  return hvp->OrI;
		case OPTYPE_XOR: return hvp->XorI;

		case OPTYPE_EQ:   return hvp->EqI;
		case OPTYPE_NE:   return hvp->NeI;
		case OPTYPE_GT:   return hvp->GtI;
		case OPTYPE_LT:   return hvp->LtI;
		case OPTYPE_GTEQ: return hvp->GtEqI;
		case OPTYPE_LTEQ: return hvp->LtEqI;

		case OPTYPE_RR: return hvp->RrI;
		case OPTYPE_LR: return hvp->LrI;
		default:
			return nullptr;
	}
}

           