// call - ModCls, functor

// モジュールクラス構築関数クラス

#include "hsp3plugin_custom.h"

#include "CFunctor.h"
#include "CModClsCtor.h"

#include "CCall.h"
#include "CCaller.h"
#include "CPrmInfo.h"

#include "cmd_sub.h"

#include "cmd_modcls.h"
#include "modcls_FlexValue.h"
#include "vt_structWrap.h"

using namespace hpimod;

//------------------------------------------------
// 構築 (ラッパー)
//------------------------------------------------
modctor_t CModClsCtor::New()
{
	return new CModClsCtor();
}

modctor_t CModClsCtor::New( stdat_t pStDat )
{
	return new CModClsCtor( pStDat );
}

modctor_t CModClsCtor::New( int modcls )
{
	return new CModClsCtor( modcls );
}

//------------------------------------------------
// 構築
//------------------------------------------------
CModClsCtor::CModClsCtor()
	: IFunctorEx()
	, mpStDat( nullptr )
	, mpPrmInfo( nullptr )
{
	initialize();
}

CModClsCtor::CModClsCtor( stdat_t pStDat )
	: IFunctorEx()
	, mpStDat( pStDat )
	, mpPrmInfo( nullptr )
{
	initialize();
}

CModClsCtor::CModClsCtor( int modcls )
	: IFunctorEx()
	, mpStDat( nullptr )
	, mpPrmInfo( nullptr )
{
	if ( modcls & AxCmd::MagicCode ) {
		if ( AxCmd::getType(modcls) != TYPE_STRUCT ) puterror( HSPERR_TYPE_MISMATCH );
		modcls = AxCmd::getCode( modcls );
	}

	stprm_t const pStPrm = getSTRUCTPRM(modcls);
	mpStDat = hpimod::STRUCTPRM_getStDat(pStPrm);

	initialize();
	return;
}

void CModClsCtor::initialize()
{
	if ( !ModCls_IsWorking() ) {
		dbgout("modcls 機能が必要");
		puterror( HSPERR_UNSUPPORTED_FUNCTION );
	}
	if ( mpStDat && getStPrm()->mptype != MPTYPE_STRUCTTAG ) puterror( HSPERR_TYPE_MISMATCH );

	createPrmInfo();
	return;
}

//------------------------------------------------
// 破棄
//------------------------------------------------
CModClsCtor::~CModClsCtor()
{
	delete mpPrmInfo; mpPrmInfo = nullptr;
//	dbgout("%s 's modclsctor destructed", mpStDat ? &ctx->mem_mds[mpStDat->nameidx] : "bottom");
	return;
}

//------------------------------------------------
// 呼び出し処理
// 
// @ これを呼ぶために用いた callerInvoke を流用してもいいのだが、
// @	thismod 引数を最初に付け加えなければいけないため、
// @	新たな caller を使っている。
// @	( CCall に「先頭に引数を追加する」ための関数を作るのは嫌 )
// @ 実質的に const な処理。
//------------------------------------------------
void CModClsCtor::call( CCaller& callerInvoke )
{
	CCall& callInvoke = callerInvoke.getCall();

	if ( isBottom() ) {
		callInvoke.setResult( ModCls::getNullmod()->pt, HSPVAR_FLAG_STRUCT );
		return;
	}

	FlexValue self { };
	FlexValue_CtorWoCtorCalling( self, mpStDat );
	FlexValue_AddRefTmp( self );			// 後でスタックに積まれる

	{
		CCaller caller;
		caller.setFunctor( AxCmd::make(TYPE_MODCMD, getCtor()->subid) );

		// thismod 引数
		caller.addArgByVal( &self, HSPVAR_FLAG_STRUCT );

		// ctor 引数を渡す
		for ( size_t i = 0; i < callInvoke.getCntArg(); ++ i ) {
			caller.addArgByRef( callInvoke.getArgPVal(i), callInvoke.getArgAptr(i) );
		}

		// 呼び出す
		caller.call();
	}

	// 返値
	callInvoke.setResult( &self, HSPVAR_FLAG_STRUCT );
	
	// 変数 self による所有の終了
	FlexValue_Release(self);
	return;
}

//------------------------------------------------
// 最初の STRUCTPRM を得る
//------------------------------------------------
stprm_t CModClsCtor::getStPrm() const
{
	return mpStDat ? &ctx->mem_minfo[ mpStDat->prmindex ] : nullptr;
}

//------------------------------------------------
// コンストラクタのユーザ定義コマンドIDを取得する
//------------------------------------------------
int CModClsCtor::getCtorId() const
{
	auto const pStPrm = getStPrm();
	return pStPrm ? AxCmd::make( TYPE_MODCMD, pStPrm->offset ) : 0;
}

stdat_t CModClsCtor::getCtor() const
{
	int const id = getCtorId();
	return ( id & AxCmd::MagicCode ) ? getSTRUCTDAT( AxCmd::getCode(id) ) : nullptr;
}

//------------------------------------------------
// 定義ID
//------------------------------------------------
int CModClsCtor::getAxCmd() const
{
	return AxCmd::make( TYPE_STRUCT, mpStDat->prmindex );
}

//------------------------------------------------
// ラベル
//------------------------------------------------
label_t CModClsCtor::getLabel() const
{
	auto const ctor = getCtor();

	return ctor ? hpimod::getLabel( ctor->otindex ) : nullptr;
}

//------------------------------------------------
// 仮引数リスト
//------------------------------------------------
CPrmInfo const& CModClsCtor::getPrmInfo() const
{
	return mpPrmInfo ? *mpPrmInfo : CPrmInfo::noprmFunc;
}

//------------------------------------------------
// 仮引数リストの生成
//------------------------------------------------
void CModClsCtor::createPrmInfo()
{
	auto const ctor = getCtor();
	if ( !ctor ) return;

	CPrmInfo const& prminfo = GetPrmInfo( ctor );	// ctor 自体の prminfo　を取得する
	int const cntPrms   = prminfo.cntPrms();		// ctor が持つ引数の数
	int const cntLocals = prminfo.cntLocals();

	// ctor の prmlist から、最初の thismod だけを除いた prmlist を作る
	CPrmInfo::prmlist_t prmlist;
	prmlist.reserve( (cntPrms - 1) + cntLocals );
	for ( int i =  1  ; i < cntPrms; ++ i ) {
		prmlist.push_back( prminfo.getPrmType(i) );
	}
	for ( int i = 0; i < cntLocals; ++ i ) prmlist.push_back( PRM_TYPE_LOCAL );

	mpPrmInfo = new CPrmInfo( &prmlist, prminfo.isFlex() );

	return;
}
