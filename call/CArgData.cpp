// CCall::CArgData
#if 0
#include <cstring>

#include "hsp3plugin_custom.h"
#include "mod_makepval.h"

#include "CCall.h"
#include "CArgData.h"
#include "CPrmInfo.h"

using namespace hpimod;

template<class T>
bool numrg(T const& val, T const& min, T const& max)
{
	return (min <= val && val <= max);
}
//#define LengthOf(arr) ( sizeof(arr) / sizeof(arr[0]) )

//#########################################################
//        コンストラクタ・デストラクタ
//#########################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CCall::CArgData::CArgData( CCall* pCall )
	: mpCall        ( pCall )
	, mpArgVal      ( &mArgVal )
	, mpIdxRef      ( &mIdxRef )
	, mpLocals      ( nullptr )
	, mCntArg       ( 0 )
{ }


//------------------------------------------------
// 複写構築
//------------------------------------------------
/*
CCall::CArgData::CArgData(const CCall::CArgData& obj)
{
	opCopy( obj );
	return;
}
//*/


//------------------------------------------------
// 解体
// 
// @ データの解放
//------------------------------------------------
CCall::CArgData::~CArgData()
{
	// 実引数をすべて解放する
	freeArgPVal();

	return;
}


//#########################################################
//        publicメンバ関数群
//#########################################################
//------------------------------------------------
// = ( 複写 )
//------------------------------------------------
/*
CCall::CArgData& CCall::CArgData::operator = (const CCall::CArgData& obj)
{
	return opCopy( obj );
}
//*/


//##########################################################
//    設定系
//##########################################################
//------------------------------------------------
// 引数を追加する ( 参照渡し )
//------------------------------------------------
void CCall::CArgData::addArgByRef( PVal* pval, APTR aptr )
{
	mpArgVal->push_back( pval );
	mpIdxRef->push_back( aptr );
	mCntArg ++;
	return;
}

//------------------------------------------------
// 引数を追加する ( 値渡し )
//------------------------------------------------
void CCall::CArgData::addArgByVal( void const* val, vartype_t vt )
{
	PVal* pvalarg  = new PVal;
	HspVarProc const* const vp = exinfo->HspFunc_getproc( vt );

	// データを複写する( 代入処理 )
	PVal_init( pvalarg, vt );
	vp->Set(
		pvalarg,
		vp->GetPtr(pvalarg),
		val
	);
//	code_setva( pvalarg, 0, pval->flag, vp->GetPtr(pval) );

	// 配列に格納
	mpArgVal->push_back(pvalarg);
	mpIdxRef->push_back(-1);
	mCntArg ++;
	return;
}

void CCall::CArgData::addArgByVal( PVal* pval )
{
	return addArgByVal( PVal_getptr(pval), pval->flag );
}

//------------------------------------------------
// 引数を追加する ( 変数複製 )
//------------------------------------------------
void CCall::CArgData::addArgByVarCopy( PVal* pval )
{
	PVal* pvalarg  = new PVal;
	HspVarProc* vp = exinfo->HspFunc_getproc( pval->flag );

	// 変数複製
	PVal_init( pvalarg, pval->flag );
	PVal_copy( pvalarg, pval );

	// 配列に格納
	mpArgVal->push_back(pvalarg);
	mpIdxRef->push_back(-1);
	mCntArg ++;
	return;
}

//------------------------------------------------
// 引数を追加する ( スキップ )
//------------------------------------------------
void CCall::CArgData::addArgSkip( int lparam )
{
	// 配列に格納
	mpArgVal->push_back( nullptr );
	mpIdxRef->push_back( lparam );
	mCntArg ++;
	return;
}

//------------------------------------------------
// ローカル変数を追加する
// 
// @ pval は prmstack 上にある PVal へのポインタ
//------------------------------------------------
void CCall::CArgData::addLocal( PVal* pval )
{
	PVal_init( pval, HSPVAR_FLAG_INT );

	if ( !mpLocals ) reserveLocals( 1 );
	mpLocals->push_back( pval );
	return;
}

//------------------------------------------------
// 引数の数を reserve する
//------------------------------------------------
void CCall::CArgData::reserveArgs( size_t cnt )
{
	if ( cnt == 0 ) return;

	mpArgVal->reserve( cnt );
	mpIdxRef->reserve( cnt );
	return;
}

void CCall::CArgData::reserveLocals( size_t cnt )
{
	if ( cnt == 0 ) return;

	if ( !mpLocals ) mpLocals = new std::vector<PVal*>();

	mpLocals->reserve( cnt );
	return;
}

//------------------------------------------------
// 実引数をすべて除去する
//------------------------------------------------
void CCall::CArgData::clearArg()
{
	// 実引数をすべて解放する
	for ( int i = 0; i < mCntArg; i ++ )
	 {
		PVal* const pArg = (*mpArgVal)[i];
		if ( !pArg ) continue;

		// 値渡し => new PVal なので、解放する
		if ( (*mpIdxRef)[i] < 0 ) {
			PVal_free( pArg );
			delete pArg;
		}
	}

	// ローカル変数を全て解放する (PVal 自体は prmstack 上にあるので解放しなくてよい)
	if ( mpLocals ) {
		for ( size_t i = 0; i < mpLocals->size(); ++ i ) {
			PVal_free( (*mpLocals)[i] );
		}
		delete mpLocals; mpLocals = nullptr;
	}

	mCntArg = 0;

	mpArgVal->clear();
	mpIdxRef->clear();
	return;
}

//################################################
//    取得系
//################################################
//------------------------------------------------
// 引数の PVal, APTR を得る
//------------------------------------------------
PVal* CCall::CArgData::getArgPVal( int iArg ) const
{
	if ( !numrg(iArg, 0, mCntArg - 1) ) return nullptr;
	return (*mpArgVal)[ iArg ];
}


APTR CCall::CArgData::getArgAptr( int iArg ) const
{
	if ( !numrg(iArg, 0, mCntArg - 1) ) return 0;

	APTR aptr = (*mpIdxRef)[ iArg ];

	return (aptr >= 0 ? aptr : 0);
}


//------------------------------------------------
// 引数の情報を取得する
//------------------------------------------------
int CCall::CArgData::getArgInfo( ARGINFOID id, int iArg ) const
{
	// 負数なら、呼び出し全体に対する情報を得る
	if ( iArg < 0 ) {
		return mCntArg;		// 引数の数

	} else {
		PVal* const pval = (*mpArgVal)[iArg];
		switch ( id ) {
			case ARGINFOID_FLAG: return pval->flag;
			case ARGINFOID_MODE: return pval->mode;
			case ARGINFOID_LEN1: return pval->len[1];
			case ARGINFOID_LEN2: return pval->len[2];
			case ARGINFOID_LEN3: return pval->len[3];
			case ARGINFOID_LEN4: return pval->len[4];
			case ARGINFOID_SIZE: return pval->size;
			case ARGINFOID_PTR : 
			{
				HspVarProc* const vp = getHvp( pval->flag );
				return reinterpret_cast<int>(
					vp->GetPtr(pval)
				);
			}
			case ARGINFOID_BYREF: return HspBool( (*mpIdxRef)[ iArg ] >= 0 );

			default:
				return 0;
		}
	}
}

//------------------------------------------------
// スキップされた引数か？ (不束縛引数)
//------------------------------------------------
bool CCall::CArgData::isArgSkipped( int iArg ) const
{
	if ( !numrg(iArg, 0, mCntArg - 1) ) return false;
	return ( getArgPVal( iArg ) == nullptr );
}

//------------------------------------------------
// ローカル変数を取得
//------------------------------------------------
PVal* CCall::CArgData::getLocal( int iLocal ) const
{
	if ( !mpLocals || !numrg<int>( iLocal, 0, mpLocals->size() - 1 ) ) return nullptr;
	return (*mpLocals)[ iLocal ];
}

//#########################################################
//        privateメンバ関数群
//#########################################################
//------------------------------------------------
// 複写
//------------------------------------------------
/*
CCall::CArgData& CCall::CArgData::opCopy(const CCall::CArgData& obj)
{
	this->~CArgData();
	mpArgVal = new std::vector<PVal*>;
	mpIdxRef = new std::vector<APTR>;

	// CCall ポインタの複写
	mpCall = obj.mpCall;

	// this データの複写
	setThis( obj.getThisPVal(), obj.getThisAptr() );

	// 実引数データの複写
	mpArgVal->reserve( obj.getCntArg() );
	mpIdxRef->reserve( obj.getCntArg() );

	for ( int i = 0; i < obj.getCntArg(); ++ i ) {
		PVal* pval = obj.getArgPVal(i);
		APTR aptr  = obj.getArgAptr(i);

		// 値渡し
		if ( aptr < 0 ) {
			addArgByVal( pval );

		// 参照渡し
		} else {
			addArgByRef( pval, aptr );
		}
	}

	return *this;
}
//*/

//------------------------------------------------
// 実引数の解放
//------------------------------------------------
void CCall::CArgData::freeArgPVal(void)
{
	clearArg();

//	delete mpArgVal; mpArgVal = nullptr;
//	delete mpIdxRef; mpIdxRef = nullptr;

	return;
}

//#########################################################
//        下請け関数群
//#########################################################
#endif