// ラムダ関数クラス

#include <map>
#include <vector>
#include <queue>

#include "CLambda.h"

#include "CCall.h"
#include "CCaller.h"
#include "CFunctor.h"

#include "CHspCode.h"

#include "iface_call.h"
#include "cmd_sub.h"

using namespace hpimod;

// 静的変数
//static std::vector<lambda_t> g_allLambdas;	// 束縛関数の実体群

//------------------------------------------------
// 構築 (ラッパー)
//------------------------------------------------
myfunc_t CLambda::New()
{
	return new CLambda();
}

//------------------------------------------------
// 構築
//------------------------------------------------
CLambda::CLambda()
	: IFunctorEx()
	, mpBody(nullptr)
	, mpPrmInfo(nullptr)
	, mpArgCloser(nullptr)
{
#ifdef DBGOUT_MY_FUNC_ADDREF_OR_RELEASE
	static int stt_id = 0; mid = ++ stt_id;
#endif
}

//------------------------------------------------
// 実引数保存オブジェクトの取得
//------------------------------------------------
CCaller* CLambda::argCloser()
{
	if ( !mpArgCloser ) mpArgCloser.swap( std::make_unique<CCaller>());
	return mpArgCloser.get();
}

//------------------------------------------------
// 仮引数リストを取得する
//------------------------------------------------
CPrmInfo const& CLambda::getPrmInfo() const
{
	return (mpPrmInfo ? *mpPrmInfo : CPrmInfo::undeclaredFunc);
}

//------------------------------------------------
// ラベルを取得する
// 
// @ コードを追加すると、ラベルが変更されるかもしれない。
//------------------------------------------------
label_t CLambda::getLabel() const
{
	return (mpBody ? mpBody->getlb() : nullptr);
}

//------------------------------------------------
// 呼び出し処理
// 
// @ 関数を呼び出す or 実行を再開する。
//------------------------------------------------
void CLambda::call( CCaller& callerGiven )
{
	label_t   const lbDst   = getLabel();
//	CPrmInfo const& prminfo = getPrmInfo();

#if 0
	{// 本体コードの列挙
		label_t mcs = lbDst;
		int _t, _v, _e;

		for ( int i = 0; ; ++ i ) {
			unsigned short a = *(mcs ++);
			_e = a & (EXFLG_0 | EXFLG_1 | EXFLG_2);
			_t = a & CSTYPE;
			if ( a & 0x8000 ) {
				_v = *(int*)mcs;
				mcs += 2;
			} else {
				_v = (int)*(mcs ++);
			}

			dbgout("code #%d (%d, %d, %X)", i, _t, _v, _e );
			if ( _t == TYPE_STRUCT ) {
				stprm_t const prm = &ctx->mem_minfo[_v];
				dbgout( "subid id: %d, mptype: %d", (int)prm->subid, (int)prm->mptype );
			}
			if ( i != 0 && _e & EXFLG_1 ) break;
		}
	}
#endif

	callerGiven.setFunctor( lbDst );

	// 保存された引数列を追加する
	if ( mpArgCloser ) {
		auto& callCloser = mpArgCloser->getCall();
		for ( size_t i = 0; i < callCloser.getCntArg(); ++ i ) {
			callerGiven.addArgByRef( callCloser.getArgPVal(i), callCloser.getArgAptr(i) );
		}
	}

	// 呼び出す
	callerGiven.call();
	return;
}

//------------------------------------------------
// スクリプトから myfunc を初期化
// 
// @ 引数式の中間コードを複写。
// @ 引数エイリアスは参照できるようにする。
// @ ( 初期化処理なのでメンバ関数にしたが、ここでいいのかと )
// @ 実行される匿名関数の prmstk は
// @	(lambda関数の引数列), (キャプチャされた変数の列), (中間計算値を持つローカル変数列)
// @ という列になる。
//------------------------------------------------
void CLambda::code_get()
{
	{
		// 複数回の初期化は許されない
		assert(!mpBody);
		mpBody.swap(std::make_unique<CHspCode>());
	}
	CHspCode& body  = *mpBody;
	int&      exflg = *exinfo->npexflg;

	// 仮引数、ローカル変数の個数
	size_t cntPrms   = 0;		
	size_t cntLocals = 0;
	
	// 「キャプチャされた変数」を参照するコードの位置 (後で修整するために位置を記憶する)
	std::vector<std::pair<stprm_t, label_t>> outerArgs;

	// 専用の命令コマンドを配置
	body.put( g_pluginType_call, CallCmdId::LambdaBody, EXFLG_1 );
	
	// 式をコピーする
	for ( int lvBracket = 0; ; ) {		
		if ( *type == TYPE_MARK ) {
			if ( *val == '(' ) lvBracket ++;
			if ( *val == ')' ) lvBracket --;
			if ( lvBracket < 0 ) break;
		}

		// 関数本体の式に追加する
		if ( lvBracket == 0 && exflg & EXFLG_2 ) {
			cntLocals++;	// 次も引数がある、つまりこの引数式は中間計算値
		}
	//	dbgout("put (%d, %d, %X)", *type, *val, exflg );

		if ( *type == TYPE_STRUCT ) {
			// 構造体パラメータが実際に指している値をコードに追加する
			auto const pStPrm = getSTRUCTPRM(*val);
			char* const prmstk = (char*)GetReferedPrmstk(pStPrm);

			// 引数を展開
			{
				char* const ptr    = prmstk + pStPrm->offset;
				int   const mptype = pStPrm->mptype;

				switch ( mptype ) {
					case MPTYPE_VAR:
					case MPTYPE_ARRAYVAR:
						return body.putVar( reinterpret_cast<MPVarData*>(ptr)->pval );

					case MPTYPE_LABEL:       return body.putVal( *reinterpret_cast<label_t*>(ptr) );
					case MPTYPE_LOCALSTRING: return body.putVal( *reinterpret_cast<char**>(ptr) );
					case MPTYPE_DNUM: return body.putVal( *reinterpret_cast<double*>(ptr) );
					case MPTYPE_INUM: return body.putVal( *reinterpret_cast<int*>(ptr) );

					case MPTYPE_SINGLEVAR:
					case MPTYPE_LOCALVAR:
					{
						auto const capturer = argCloser();

						// 変数要素は、リテラル値で記述できないので、lambda関数の prmstk に乗せるために保存する
						if ( mptype == MPTYPE_SINGLEVAR ) {
							auto const vardata = reinterpret_cast<MPVarData*>(ptr);
							capturer->addArgByRef( vardata->pval, vardata->aptr );

						// ローカル変数は、実行中に消滅するかもなのでコピーを取る
						} else {
							auto const pval = reinterpret_cast<PVal*>(ptr);
							capturer->addArgByVarCopy( pval );
						}

						// キャプチャしたものを記録しておく
						outerArgs.push_back({ pStPrm, body.getlbNow() });
						body.put( TYPE_STRUCT, -1, exflg );
						// @ これが lamda 関数の何番目の実引数になるかは、仮引数の数が確定するまで分からない
						// @ short では収まらない値かもなので、(-1 にして) int サイズを使用する
						break;
					}
					default: dbgout("mptype = %d", mptype );
						break;
				}
			}
			code_next();

		} else if ( *type == g_pluginType_call && *val == CallCmdId::PrmOf ) {
			// 仮引数プレースホルダ [ call_prmof ( (引数番号) ) ]
			int const exflg_here = exflg;
			code_next();
			 
			if ( !code_next_expect( TYPE_MARK, '(' ) ) puterror( HSPERR_SYNTAX );

			int const iPrm = code_geti();
			if ( iPrm < 0 ) puterror(HSPERR_ILLEGAL_FUNCTION);

			if ( !code_next_expect( TYPE_MARK, ')' ) ) puterror( HSPERR_SYNTAX );

			// 仮引数の数を確保
			cntPrms = std::max<size_t>(cntPrms, iPrm + 1);

			// 対応する実引数を取り出すコード「argv(n)」を出力
			body.put( g_pluginType_call, CallCmdId::ArgV, exflg_here );
			body.put( TYPE_MARK, '(', 0 );
			body.putVal( iPrm );
			body.put( TYPE_MARK, ')', 0 );

		} else if ( *type == g_pluginType_call && *val == CallCmdId::Lambda ) {
			// lambda 関数
			// @ これの内側にある構造体パラメータや仮引数プレースホルダを今は無視するために、引数式を単純複写する
			body.put( *type, *val, exflg );
			code_next();

			if ( *type == TYPE_MARK && *val == '(' ) {
				for ( int lvBracket = 0; ; ) {		// 無限ループ
					if ( *type == TYPE_MARK ) {
						if ( *val == '(' ) lvBracket ++;
						if ( *val == ')' ) lvBracket --;
					}
					body.put( *type, *val, exflg );
					code_next();
					if ( lvBracket == 0 ) break;
				}
			}

		} else {
			// (その他)
			body.put( *type, *val, exflg );
			code_next();
		}
	}

//	if ( exflg & EXFLG_2 ) puterror( HSPERR_TOO_MANY_PARAMETERS );

	// コードの先読みによるオーバーランを防ぐための番兵
	body.putReturn();	
	body.putReturn();

	// lambda 関数を呼ぶための仮引数リストの構築
	// 次の prmlist とは共用しない (キャプチャ変数などを引数に指定できてしまうため)。
	{
		CPrmInfo::prmlist_t prmlistBase(cntPrms, PRM_TYPE_ANY);

		assert(!mpPrmInfo);
		mpPrmInfo.swap(std::make_unique<CPrmInfo>(&prmlistBase));
	}

	// 内部から lamda の本体 call するときの仮引数リストの構築
	{
		// 仮引数形式：「lambda引数(_pN) + キャプチャ変数 + ローカル変数(_vN)」
		// @ すべて可変長引数で処理したいところだが、prmstack に積まなきゃいけないので (ローカル変数を積むなら可変長部分は積めない、積んでしまうと local にエイリアスでアクセスできない)

		CPrmInfo::prmlist_t prmlist;
		prmlist.resize(cntPrms + outerArgs.size() + cntLocals);

		// lamda 引数
		std::fill(prmlist.begin(), prmlist.begin() + cntPrms, PRM_TYPE_ANY);

		// キャプチャ値
		for ( size_t i = 0; i < outerArgs.size(); ++i ) {
			prmlist.push_back(
				(outerArgs[cntPrms + i].first->mptype == MPTYPE_SINGLEVAR)
				? PRM_TYPE_VAR
				: PRM_TYPE_ARRAY
			);
		}

		// ローカル変数
		std::fill(prmlist.end() - cntLocals, prmlist.end(), PRM_TYPE_LOCAL);

		DeclarePrmInfo(body.getlb(), CPrmInfo(&prmlist, true));
	}

	// ラムダ式中に含まれる、「キャプチャ変数を参照している TYPE_STRUCT コード」の code 値を補完する
	{
		// lamda 引数の後ろ
		int offset = sizeof(MPVarData) * cntPrms;

		for ( size_t i = 0; i < outerArgs.size(); ++ i ) {
			*(int*)(outerArgs[i].second + 1) = body.putDsStPrm(
				STRUCTPRM_SUBID_STACK,
				((outerArgs[i].first->mptype) == MPTYPE_SINGLEVAR ? MPTYPE_SINGLEVAR : MPTYPE_ARRAYVAR),
				offset
			);
			offset += sizeof(MPVarData);
		}
	}
	return;
}

//------------------------------------------------
// 
//------------------------------------------------

//------------------------------------------------
// 
//------------------------------------------------


