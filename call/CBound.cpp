// 束縛関数クラス

#include <map>
#include <vector>
#include <queue>

#include "CBound.h"

#include "CCall.h"
#include "CCaller.h"
#include "CPrmInfo.h"

using namespace hpimod;

template<class TLhs, class TRhs>
struct LessOfPairLhsPriority	// for createRemains() : 「左辺を優先度とする対の比較 "<"」
	: public std::binary_function< std::pair<TLhs, TRhs>, std::pair<TLhs, TRhs>, int >
{
	typedef const std::pair<TLhs, TRhs>& prm_t;
	bool operator ()( prm_t lhs, prm_t rhs ) const {		// lhs の方が小さいか？
		return (lhs.first != rhs.first) ? lhs.first > rhs.first : lhs.second > rhs.second;
	}
};

// 静的変数
//static std::vector<bound_t> g_allBounds;	// 束縛関数の実体群

//------------------------------------------------
// 構築 (ラッパー)
//------------------------------------------------
bound_t CBound::New()
{
	return new CBound();
}

//------------------------------------------------
// 構築
//------------------------------------------------
CBound::CBound()
	: IFunctor()
	, mpCaller( new CCaller( CCaller::CallMode::Bind ) )
	, mpPrmIdxAssoc( new prmidxAssoc_t() )
{}

//------------------------------------------------
// 破棄
//------------------------------------------------
CBound::~CBound()
{
	delete mpCaller;      mpCaller      = nullptr;
	delete mpRemains;     mpRemains     = nullptr;
	delete mpPrmIdxAssoc; mpPrmIdxAssoc = nullptr;
	return;
}

//------------------------------------------------
// 束縛設定
//------------------------------------------------
void CBound::bind()
{
	createRemains();
	return;
}

//------------------------------------------------
// 束縛関数の仮引数リストを生成する
// 
// @ 前からのみ束縛できる。
//------------------------------------------------
void CBound::createRemains()
{
	CCaller const&  callerBinder = *mpCaller;
	CCall const&    callBinder   = callerBinder.getCall();
	CPrmInfo const& srcPrminfo   = callBinder.getPrmInfo();

	int const cntPrms = srcPrminfo.cntPrms();
	int const cntArgs = callerBinder.getCall().getCntArg();
//	dbgout("(prms, args) = (%d, %d) -> %d", cntPrms, cntArgs, cntPrms - cntArgs );

	// 実引数リストから残引数リストを生成する
	CPrmInfo::prmlist_t prmlist;

	auto const addRemainPrm = [&]( int i ) {		// 元の仮引数 [i] を残引数として追加する関数
		mpPrmIdxAssoc->push_back( i );
		prmlist.push_back( srcPrminfo.getPrmType( i ) );
	};

	{
		// 不束縛引数 (引数が指定されたもののうち、nobind であるもの)
		{
			// [ややこしいので詳細メモ]
			// @ todo: argbind() の引数に不束縛引数 nobind が指定されているものを検索し、
			// @	それを優先順に並べ替える。
			// @	そして、優先値の低いものから先に、残引数リストに追加する。
			// @	優先値が等しい場合は、元々の引数番号が小さい順に追加する。
			// @ impl: 対 < int 優先値, int 引数番号 > を要素とする優先度付きキュー argNobind に
			// @	すべての不束縛引数を追加する。
			// @	優先度付きキューから値を順次 pop すると、引数番号が優先順 (優先値の小さい順) で得られる。
			// @	( キューの優先度は、対の左辺(優先度)の大きさで決まる; ように LessOfPairLhsPriority<> を実装した )
			// @	( std::priority_queue<T> は降順(優先度の高い順)で得られる )

			typedef std::pair<int, int> thePair_t;
			std::priority_queue< thePair_t, std::vector<thePair_t>, LessOfPairLhsPriority<int, int>>
				argsNobind;		// 不束縛引数リスト 

			for ( int i = 0; i < cntArgs; ++ i ) {
				if ( callBinder.isArgSkipped(i) ) {
					// < 優先値, 仮引数での引数番号 >
					argsNobind.push({ callBinder.getArgAptr(i), i });
				//	dbgout("nobind-arg; push (%d, %d)", callBinder.getArgAptr(i), i);
				}
			}

			while ( !argsNobind.empty() ) {
			//	dbgout("nobind-arg; pop (%d, %d)", argsNobind.top().first, argsNobind.top().second);
				addRemainPrm( argsNobind.top().second );
				argsNobind.pop();
			}
		}

		// 後半部分 (仮引数のうち、実引数とも不束縛とも指定されなかった部分)
		for ( int i = cntArgs; i < cntPrms; ++ i ) {
			addRemainPrm( i );
	//		dbgout("remain #%d : %d", i, srcPrmInfo().getPrmType(i) );
		}
	}

	// メンバ変数に設定する
	mpRemains = new CPrmInfo( &prmlist, srcPrminfo.isFlex() );

	return;
};

//------------------------------------------------
// 呼び出し処理
// 
// @ caller の引数と mpCaller の引数をまとめて、
// @	新たに生成した CCaller で被束縛関数を呼び出す。
// @prm callerRemain:
// @	{ functor: this, args: this.remains }
// @	束縛関数の呼び出しのために、外部で生成されたもの
//------------------------------------------------
void CBound::call( CCaller& callerRemain )
{
	CCall& callBinder = mpCaller->getCall();
	CCall& callRemain = callerRemain.getCall();

	CCaller caller;
	{
		caller.setFunctor( callBinder.getFunctor() );	// 被束縛関数

		// 引数の準備
		{
			size_t const cntPrmsBinder = callBinder.getPrmInfo().cntPrms();	// 被束縛関数の仮引数の数
			size_t const cntArgsBinder = callBinder.getCntArg();
			size_t const cntArgsRemain = callRemain.getCntArg();
			size_t const cntPrmsAll    = cntArgsBinder;	// 被束縛関数に与えられる実引数の数 (可変部を除く)
			
			// 最終的に被束縛関数に与える実引数リスト (可変部は後で拡張)
			std::vector< std::pair<PVal*, APTR> >
				argsAll( std::max(cntPrmsBinder, cntPrmsAll) );

			// 束縛引数を追加 (mpCaller が持つ引数を参照渡しする)
			for ( size_t i = 0; i < cntArgsBinder; ++ i ) {
				if ( callBinder.isArgSkipped(i) ) continue;
				PVal* const pval = callBinder.getArgPVal(i);
				APTR  const aptr = callBinder.getArgAptr(i);

			//	dbgout("set #%d", i);
				argsAll[i] = { pval, aptr };
			}

			// 余剰引数を追加 (callerRemain が持つ引数を参照渡しする)
			for ( size_t i = 0; i < cntArgsRemain; ++ i ) {
				if ( callRemain.isArgSkipped(i) ) continue;
				PVal* const pval = callRemain.getArgPVal(i);
				APTR  const aptr = callRemain.getArgAptr(i);
			//	dbgout("aptr = %d", aptr);
				auto const&& elem = std::make_pair(pval, aptr);

				// 不束縛引数を埋める引数 (残引数の一部)
				if ( i < mpPrmIdxAssoc->size() ) {
					// mpPrmIdxAssoc: 残引数の引数番号→被束縛関数の仮引数の引数番号
					argsAll[ mpPrmIdxAssoc->at(i) ] = std::move(elem);

				// 可変長部分の追加引数
				} else {
					argsAll.push_back( std::move(elem) );
				}
			}

			// 合成引数列を参照渡しで追加する
			for ( size_t i = 0; i < argsAll.size(); ++ i ) {
				caller.addArgByRef( argsAll[i].first, argsAll[i].second );
			}
		}

		// 呼び出す
		caller.call();

		// 返値を callerRemain に転送する
		callRemain.setRetValTransmit( caller.getCall() );
	}
	return;
}

//------------------------------------------------
// 束縛解除
// 
// @ 被束縛関数が束縛関数ならその unbind() を返却する。
// @	そうでなければ被束縛関数を返却する。
//------------------------------------------------
functor_t const& CBound::unbind() const
{
	auto&      bound  = getBound();
	auto const casted = bound.castTo<bound_t>();

	if ( !casted ) {
		return bound;
	} else {
		return casted->unbind();
	}
}

//------------------------------------------------
// 被束縛関数
//------------------------------------------------
functor_t const& CBound::getBound() const
{
	return mpCaller->getCall().getFunctor();
}


//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// 
//------------------------------------------------
