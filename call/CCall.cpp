// CCall

#include "hsp3plugin_custom.h"
#include "mod_makepval.h"

#include "CCall.h"
#include "CArgData.h"
#include "CPrmInfo.h"
#include "CPrmStkCreator.h"

#include "CCaller.h"
#include "CBound.h"

#include "cmd_sub.h"

using namespace hpimod;

//##############################################################################
//                定義部 : CCall
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CCall::CCall()
	: mFunctor()
	, mpArg         ( nullptr )
	, mpRetVal      ( nullptr )
	, mbRetValOwn   ( false )
	, mpOldPrmStack ( nullptr )
	, mpPrmStack    ( nullptr )
	, mbUnCallable  ( false )
{
	mpArg = new CArgData( this );
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CCall::~CCall()
{
	// 実引数データを解放 (prmstack 上のデータも含む)
	delete mpArg; mpArg = nullptr;

	// prmstack の解放
	destroyPrmStack();

	// 返値データを解放
	freeRetVal();

	return;
}

//##########################################################
//    コマンド処理
//##########################################################
//------------------------------------------------
// エイリアス
//------------------------------------------------
void CCall::alias(PVal* pval, int iArg) const
{
	if ( !numrg<size_t>(iArg, 0, mpArg->getCntArg() - 1) ) return;

	// エイリアス変数を、引数値のクローンとする
	PVal*  pvalArg = mpArg->getArgPVal( iArg );		// 引数の変数
	APTR   aptr    = mpArg->getArgAptr( iArg );

	// 変数クローン化
	PVal_cloneVar( pval, pvalArg, aptr );

	return;
}

//------------------------------------------------
// 実引数データを取得する
// 
// @result : 実引数を格納した PVal へのポインタ
//------------------------------------------------
PVal* CCall::getArgv(int iArg) const
{
	PVal* pval = mpArg->getArgPVal( iArg );

	if ( !pval ) {
		pval = PVal_getDefault();	// 既定値を使う
	}

	return pval;
}

//##########################################################
//    動作
//##########################################################
//------------------------------------------------
// call
// 
// @ 複数回呼ばれることに注意していない。
// @	そうする場合は CPrmStack を使い回すべき。
//------------------------------------------------
void CCall::call( CCaller& caller )
{
	if ( mFunctor.isNull() || mbUnCallable ) {
		dbgout("呼び出し失敗\nもしかして: 関数が未設定な functor, nobind 引数が与えられた, etc");
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}

	mFunctor->call( caller );
	return;
}

//------------------------------------------------
// ラベルの呼び出し
//------------------------------------------------
void CCall::callLabel( label_t lb )
{
	pushPrmStack();

	code_call( lb );	// サブルーチン呼び出し

	popPrmStack();
	return;
}

//------------------------------------------------
// 呼び出す関数の設定
//------------------------------------------------
void CCall::setFunctor( functor_t const& functor )
{
	mFunctor = functor;

	// 引数の数を reserve
	mpArg->reserveArgs( getPrmInfo().cntPrms() );
	mpArg->reserveLocals( getPrmInfo().cntLocals() );
	return;
}

//################################################
//    実引数データの設定
//################################################
//------------------------------------------------
// 引数を追加する ( 参照渡し )
//------------------------------------------------
void CCall::addArgByRef( PVal* pval )
{
	return addArgByRef( pval, pval->offset );
}

void CCall::addArgByRef( PVal* pval, APTR aptr )
{
	mpArg->addArgByRef( pval, aptr );
	return;
}

//------------------------------------------------
// 引数を追加する ( 値渡し )
//------------------------------------------------
void CCall::addArgByVal( void const* val, vartype_t vt )
{
	mpArg->addArgByVal( val, vt );
	return;
}

void CCall::addArgByVal(PVal* pval)
{
	mpArg->addArgByVal( pval );
	return;
}

//------------------------------------------------
// 引数を追加する ( 変数複製 )
//------------------------------------------------
void CCall::addArgByVarCopy( PVal* pval )
{
	return mpArg->addArgByVarCopy( pval );
}

//------------------------------------------------
// 引数を追加する ( スキップ )
//------------------------------------------------
void CCall::addArgSkip( int lparam )
{
	mbUnCallable = true;
	mpArg->addArgByRef( nullptr, lparam );
	return;
}

//------------------------------------------------
// ローカル変数を追加する
//------------------------------------------------
void CCall::addLocal( PVal* pval )
{
	mpArg->addLocal( pval );
	return;
}

//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// 足りない引数を既定値で補う
//------------------------------------------------
void CCall::completeArg(void)
{
	CPrmInfo const& prminfo = getPrmInfo();

	// 可変長引数ではないのに、引数が多すぎる
	if ( !prminfo.isFlex() && prminfo.cntPrms() < mpArg->getCntArg() ) {
		puterror( HSPERR_TOO_MANY_PARAMETERS );
	}

	// 既定値で埋める、埋まらなければエラー
	for ( size_t i = mpArg->getCntArg()
		; i < prminfo.cntPrms()
		; ++ i
	) {
		// 省略値 or エラー
		mpArg->addArgByVal(
			prminfo.getDefaultArg(i)
		);
	}
	return;
}

//------------------------------------------------
// 実引数情報をすべて除去する
//------------------------------------------------
void CCall::clearArg()
{
	mpArg->clearArg();
	return;
}

//################################################
//    仮引数情報
//################################################


//################################################
//    返値
//################################################
//------------------------------------------------
// 返値の設定
//------------------------------------------------
void CCall::setResult(void* pRetVal, vartype_t vt)
{
	if ( !mpRetVal && !mbRetValOwn ) {
		mpRetVal = new PVal;
		PVal_init( mpRetVal, vt );
		mbRetValOwn = true;
	}

	// 代入
	code_setva( mpRetVal, 0, vt, pRetVal );
	return;
}

// 転送される
void CCall::setRetValTransmit( CCall& callSrc )
{
	std::swap( mpRetVal,    callSrc.mpRetVal );
	std::swap( mbRetValOwn, callSrc.mbRetValOwn );
	return;
}

//################################################
//    prmstack
//################################################
//------------------------------------------------
// prmstack push
//------------------------------------------------
void CCall::pushPrmStack()
{
	mpOldPrmStack = ctx->prmstack;	// 以前の prmstack の状態

	if ( !mpPrmStack ) createPrmStack();

	// prmstack の変更
	ctx->prmstack = mpPrmStack;
	return;
}

//------------------------------------------------
// prmstack pop
//------------------------------------------------
void CCall::popPrmStack()
{
	ctx->prmstack = mpOldPrmStack;		// prmstack を戻す
	return;
}

//------------------------------------------------
// prmstack 生成 / 破棄
//------------------------------------------------
void CCall::createPrmStack()
{
	destroyPrmStack();
	mpPrmStack = newPrmStack( this, hspmalloc );
	return;
}

void CCall::destroyPrmStack()
{
	if ( mpPrmStack ) {
		hspfree( mpPrmStack ); mpPrmStack = nullptr;
	}
	return;
}

//------------------------------------------------
// CPrmStk の設定
// 
// @static
// @ 既宣言 => 仮引数タイプに応じた形式で積む、可変長部分は積まない (無意味なので)。
// @ 未宣言 => すべて積む。
// @ 引数の数については、実引数取得時に検査済み。
// @ バッファは受け取った側が解放すること。
//------------------------------------------------
void* CCall::newPrmStack( CCall* const pCall, char*(*pfAllocator)(int) )
{
	CPrmInfo const& prminfo = pCall->getPrmInfo();

	// 仮引数リスト通りに積む ( HSP本体と同じ形式 )
	if ( &prminfo != &CPrmInfo::undeclaredFunc ) {
		size_t const sizeStack = prminfo.getStackSize();
		size_t const cntPrms   = prminfo.cntPrms();

		CPrmStkCreator prmstk( (*pfAllocator)( sizeStack ), sizeStack );

		for ( size_t i = 0; i < cntPrms; ++ i )
		 {
			PVal* const pval = pCall->getArgPVal(i);
			APTR  const aptr = pCall->getArgAptr(i);		// 値渡し引数なら負

			int const prmtype = prminfo.getPrmType(i);

			switch ( prmtype ) {
				case HSPVAR_FLAG_LABEL:		// HSP側では未実装だが
					prmstk.pushValue( *reinterpret_cast<label_t*>(pval->pt) );
					break;

				case HSPVAR_FLAG_STR:
					prmstk.pushValue( pval->pt );
					break;

				case HSPVAR_FLAG_DOUBLE:
					prmstk.pushValue( *reinterpret_cast<double*>(pval->pt) );
					break;

				case HSPVAR_FLAG_INT:
					prmstk.pushValue( *reinterpret_cast<int*>(pval->pt) );
					break;

				case PRM_TYPE_VAR:
				case PRM_TYPE_ARRAY:
				case PRM_TYPE_ANY:
					prmstk.pushPVal( pval, aptr );
					break;

				case PRM_TYPE_MODVAR:
				{
					auto const fv = reinterpret_cast<FlexValue*>( PVal_getPtr(pval) );
					prmstk.pushThismod( pval, aptr, FlexValue_getModuleTag(fv)->subid );
					break;
				}
				default:
					// その他の型タイプ値
					if ( HSPVAR_FLAG_INT < prmtype && prmtype < (HSPVAR_FLAG_USERDEF + ctx->hsphed->max_varhpi) ) {
						if ( pval->flag != prmtype ) puterror( HSPERR_TYPE_MISMATCH );
						prmstk.pushPVal( pval, aptr );
						break;
					}

					// 異常
					puterror( HSPERR_ILLEGAL_FUNCTION );
			}
		}

		// ローカル変数を積む
		size_t const cntLocals = prminfo.cntLocals();
		for ( size_t i = 0; i < cntLocals; ++ i ) {
			pCall->addLocal( prmstk.pushLocal() );
		}

		return prmstk.getptr();

	// 仮引数なし => 与えられた引数をすべて積む
	} else {
		size_t const cntArgs   = pCall->getCntArg();
		size_t const sizeStack = prminfo.getStackSizeWithFlex( cntArgs );

		CPrmStkCreator prmstk( (*pfAllocator)( sizeStack ), sizeStack );

		for ( size_t i = 0; i < cntArgs; ++ i ) {
			PVal* const pval = pCall->getArgPVal( i );

			// すべて var, array の形式で渡す
			prmstk.pushPVal( pval, pval->offset );
		}

		return prmstk.getptr();
	}
}

//##########################################################
//    取得
//##########################################################
//------------------------------------------------
// 
//------------------------------------------------

//################################################
//    実引数データの取得
//################################################
//------------------------------------------------
// 実引数の数
//------------------------------------------------
size_t CCall::getCntArg() const
{
	return mpArg->getCntArg();
}

//------------------------------------------------
// 実引数データの pval
//------------------------------------------------
PVal* CCall::getArgPVal(int iArg) const
{
	return mpArg->getArgPVal( iArg );
}


//------------------------------------------------
// 実引数データの pval の aptr
//------------------------------------------------
APTR CCall::getArgAptr(int iArg) const
{
	return mpArg->getArgAptr( iArg );
}


//------------------------------------------------
// 実引数情報
//------------------------------------------------
int CCall::getArgInfo(ARGINFOID id, int iArg) const
{
	// 負数なら、呼び出し全体に関する情報を得る
	if ( iArg < 0 ) {
		return mpArg->getArgInfo(id);

	// 引数ごとの情報を得る
	} else {
		// id の範囲チェック
		if ( !numrg<int>( id, 0, ARGINFOID_MAX - 1 ) ) {
			puterror( HSPERR_ILLEGAL_FUNCTION );	// "引数の値が異常です"
		}

		return mpArg->getArgInfo(id, iArg);
	}
}

//------------------------------------------------
// スキップされた引数か？
//------------------------------------------------
bool CCall::isArgSkipped( int iArg ) const
{
	return mpArg->isArgSkipped(iArg);
}

//------------------------------------------------
// ローカル変数を取得
//------------------------------------------------
PVal* CCall::getLocal( int iLocal ) const
{
	return mpArg->getLocal(iLocal);
}

//################################################
//    仮引数情報の取得
//################################################
//------------------------------------------------
// 仮引数タイプ
//------------------------------------------------
int CCall::getPrmType(int iPrm) const
{
	return getPrmInfo().getPrmType(iPrm);
}


//------------------------------------------------
// デフォルト値
//------------------------------------------------
PVal* CCall::getDefaultArg(int iPrm) const
{
	return getPrmInfo().getDefaultArg(iPrm);
}


//------------------------------------------------
// 正しい引数かどうか
//------------------------------------------------
void CCall::checkCorrectArg(PVal const* pvArg, int iArg, bool bByRef) const
{
	getPrmInfo().checkCorrectArg( pvArg, iArg, bByRef );
	return;
}


//################################################
//    返値データの取得
//################################################
//------------------------------------------------
// 返値の PVal を得る
//------------------------------------------------
PVal* CCall::getRetVal() const
{
	return mpRetVal;
}

//##########################################################
//    内部メンバ関数
//##########################################################
//------------------------------------------------
// 返値(mpRetVal)の解放
//------------------------------------------------
void CCall::freeRetVal()
{
	if ( !mpRetVal ) return;

	if ( mbRetValOwn ) {
		// 中身の解放
		PVal_free( mpRetVal );

		// mpRetVal 自体の解放
		delete mpRetVal; 
	}

	mpRetVal    = nullptr;
	mbRetValOwn = false;
	return;
}

//------------------------------------------------
// 
//------------------------------------------------
