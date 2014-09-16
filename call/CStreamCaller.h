// ストリーム呼び出しオブジェクト

#ifndef IG_CLASS_STREAM_CALLER_H
#define IG_CLASS_STREAM_CALLER_H

// 「呼び出しの途中」を保存するオブジェクト

#include "hsp3plugin_custom.h"
#include "IFunctorEx.h"

class CCaller;
class CStreamCaller;

using stream_t = CStreamCaller*;

class CStreamCaller
	: public IFunctorEx
{
	// メンバ変数
private:
	CCaller* mpCaller;		// ストリームに追加された引数を保持する

	// 構築
private:
	CStreamCaller();
	~CStreamCaller();

public:
	void call( CCaller& callerRemain );		// (束縛引数解決処理 + 呼び出し)

	CCaller* getCaller()  const { return mpCaller; }
	label_t   getLabel() const;
	int       getAxCmd() const;
	int       getUsing() const;

	CPrmInfo const& getPrmInfo() const;			// 仮引数

	// ラッパー
	static stream_t New();
};

#endif
