// コルーチンクラス
#if 0
// IFunctor を継承する必要がある。

#ifndef IG_CLASS_CO_ROUTINE_H
#define IG_CLASS_CO_ROUTINE_H

#include <vector>

#include "hsp3plugin_custom.h"
#include "IFunctor.h"
#include "Functor.h"

//	#define DBGOUT_CO_ROUTINE_ADDREF_OR_RELEASE	// AddRef, Release を dbgout で報告する

class CCoRoutine;
class CCaller;

using coroutine_t = CCoRoutine*;

class CCoRoutine
	: public IFunctor
{
	// メンバ変数
private:
	CCaller* mpCaller;		// 継続している呼び出し
	functor_t mNext;			// 次に呼び出すラベル

	CCaller const* mpCallerGiven;	// 実際の呼び出しへの参照

	static PVal const* stt_pvNextVar;	// next を受け取る変数への参照

	// 構築
private:
	CCoRoutine();
	~CCoRoutine();

public:
	CCaller* getCaller()  const { return mpCaller; }
	CPrmInfo const& getPrmInfo() const;

	// 継承
	label_t getLabel() const { return mNext.getLabel(); }
	int     getAxCmd() const { return mNext.getAxCmd(); }
	int     getUsing() const { return 1; }

	// 動作
	void call( CCaller& callerGiven );		// 追加引数

	// ラッパー
	static coroutine_t New();

	static void setNextVar( PVal const* pv )	// co_yield_impl 実行時にコルーチンを参照する方法はない (実際に呼ばれているのは実体なわけだし)
	{ stt_pvNextVar = pv; }
};

#endif

#endif