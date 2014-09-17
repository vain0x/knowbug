// ラムダ関数クラス
#if 0
#ifndef IG_CLASS_LAMBDA_FUNC_H
#define IG_CLASS_LAMBDA_FUNC_H

#include "hsp3plugin_custom.h"
#include <vector>
#include <memory>

#include "CCaller.h"
#include "CHspCode.h"
#include "CPrmInfo.h"
#include "IFunctor.h"

class CLambda;
using myfunc_t = CLambda*;

class CLambda
	: public IFunctor
{
	// メンバ変数
private:
	// 自作関数の本体コード、仮引数
	std::unique_ptr<CHspCode> mpBody;		
	std::unique_ptr<CPrmInfo> mpPrmInfo;

	// 生成環境の実引数を保存するもの
	std::unique_ptr<CCaller> mpArgCloser;

	// 構築
private:
	CLambda();

	CCaller* argCloser();

public:
	void call( CCaller& caller );

	void code_get();

	CHspCode const& getBody()    const { return *mpBody; }
	CPrmInfo const& getPrmInfo() const;
	label_t         getLabel()   const;

	int getUsing() const { return 1; }

	// ラッパー
	static myfunc_t New();
};

#endif

#endif