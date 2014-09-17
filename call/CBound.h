// 束縛関数クラス
#if 0
#ifndef IG_CLASS_BOUND_H
#define IG_CLASS_BOUND_H

#include "hsp3plugin_custom.h"
#include <vector>

#include "Functor.h"
#include "IFunctor.h"

//	#define DBGOUT_BOUND_ADDREF_OR_RELEASE	// AddRef, Release を dbgout で報告する

class CCaller;
class CPrmInfo;

class CBound;
using bound_t = CBound*;

class CBound
	: public IFunctor
{
	using prmidxAssoc_t = std::vector<int>;

	// メンバ変数
private:
	CCaller*  mpCaller;				// 束縛引数を保持する
	CPrmInfo* mpRemains;			// 残引数 (CBound が生成する)

	prmidxAssoc_t* mpPrmIdxAssoc;	// 残引数と元引数の引数番号の対応を取る (各要素: 元引数の引数番号)

	// 構築
private:
	CBound();
	~CBound();

	void createRemains();

public:

	CCaller*  getCaller()  const { return mpCaller; }
	CPrmInfo& getPrmInfo() const { return *mpRemains; }

	// 継承
	label_t getLabel() const { return getBound()->getLabel(); }
	int     getAxCmd() const { return getBound()->getAxCmd(); }
	int     getUsing() const { return 1; }

	functor_t const& unbind() const;

	// 動作
	void bind();							// 束縛処理
	void call( CCaller& callerRemain );		// (束縛引数解決処理 + 呼び出し)

	// ラッパー
	static bound_t New();

private:
	functor_t const& getBound() const;		// 被束縛関数

};

#endif
#endif