// call クラス
#if 0
#ifndef IG_CLASS_CALL_H
#define IG_CLASS_CALL_H

/**
@summary:
	call に関するすべての情報を管理するクラス。
	CCaller によってのみ生成、解体、所有される。

@role:
	1. call に関するすべての情報の関連づけ。
		0. 呼び出し先   functor_t
			これを設定しないと引数を与えられない。
		1. 仮引数情報   CPrmInfo
			functor_t から間接的に得られる。
		2. 実引数データ CArgData
		3. 返値情報 (省略可)
			call_retval によって与えられた返値の保存を担当する。
			return によって得られる返値は caller が処理してくれる。
	2. prmstk の管理。
		ctx->prmstack への操作、旧 prmstack の保持も、これが行う。
		prmstk 上のデータ (char* や local の PVal) は、CArgData が管理するので解放する必要はない。
**/

#include "hsp3plugin_custom.h"
using namespace hpimod;

#include "CCall.const.h"
#include "Functor.h"

class CCaller;
class CPrmInfo;

class CCall
{
private:
	class CArgData;

	// メンバ変数
private:
	functor_t  mFunctor;		// 関数
	CArgData* mpArg;		// 実引数データ
	PVal*     mpRetVal;		// 返値 (実行中に与えられた返値の保存)
	bool      mbRetValOwn;	// mpRetVal が、この Call が所有している PVal か否か

	// prmstack 関連
	void* mpOldPrmStack;	// 以前の ctx->prmstack 値
	void* mpPrmStack;		// 独自の prmstack

	// その他
	bool mbUnCallable;		// 呼び出し不可能か？

public:
	//--------------------------------------------
	// 構築・解体
	//--------------------------------------------
	CCall();
	~CCall();

	//--------------------------------------------
	// コマンド処理
	//--------------------------------------------
	void alias( PVal* pval, int iArg ) const;
	PVal* getArgv( int iArg ) const;

	//--------------------------------------------
	// 動作
	//--------------------------------------------
	void call( CCaller& caller );
	void callLabel( label_t lb );		// ラベル関数の実行動作 (functor からのみ起動可能)

	// 呼び出し関数の設定
	void setFunctor( functor_t const& functor );

	// 実引数データの設定
	void addArgByVal( void const* val, vartype_t vt );
	void addArgByVal( PVal* pval );
	void addArgByRef( PVal* pval );
	void addArgByRef( PVal* pval, APTR aptr );
	void addArgByVarCopy( PVal* pval );
	void addArgSkip ( int lparam = 0 );
	void addLocal   ( PVal* pval );

	void completeArg();
	void clearArg();

	// 返値データの設定
	void setResult( void* pRetVal, vartype_t vt );
	void setRetValTransmit( CCall& callSrc );		// 転送(繰り上げ)

	// prmstack (ラベル関数のとき)
private:
	void pushPrmStack();
	void popPrmStack();

	void createPrmStack();
	void destroyPrmStack();

	static void* newPrmStack( CCall* _this, char*(*pfAllocator)(int) );

	//--------------------------------------------
	// 取得
	//--------------------------------------------
public:
	functor_t const& getFunctor()  const { return mFunctor; }

	// 仮引数情報の取得
	CPrmInfo const& getPrmInfo() const { return mFunctor->getPrmInfo(); }
	int   getPrmType    ( int iPrm ) const;
	PVal* getDefaultArg ( int iPrm ) const;
	void checkCorrectArg( PVal const* pvArg, int iArg, bool bByRef = false ) const;

	// 実引数データの取得
	size_t getCntArg() const;
	PVal* getArgPVal( int nArg ) const;
	APTR  getArgAptr( int iArg ) const;
	int   getArgInfo( ARGINFOID id, int iArg = -1 ) const;
	bool  isArgSkipped( int iArg ) const;
	PVal* getLocal( int iLocal ) const;

	// 返値データの取得
	PVal* getRetVal() const;

private:
	// 後始末
	void freeRetVal();

private:
	// 封印
	CCall( CCall const& );
	CCall& operator = ( CCall const& );
};

#endif

#endif