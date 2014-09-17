// 関数子インターフェース

// 「呼び出せるもの」を表すために、プラグイン側で作成するクラスたちの基底クラス。
// 例えば、CLabelFunc, CDeffunc, CBound, CCoRoutine, CLambda などに継承されている。

// Managed<> に入れて functor_t として使われる。

#ifndef IG_INTERFACE_FUNCTOR_H
#define IG_INTERFACE_FUNCTOR_H

#include "hsp3plugin_custom.h"
using namespace hpimod;

class CPrmInfo;
class CCaller;

//------------------------------------------------
// 関数子を表すクラス
// 
// @ 継承して使う。
// @ ここでは既定動作を定義している。
//------------------------------------------------
class IFunctor
{
protected:
	IFunctor() = default;

public:
	virtual ~IFunctor() { clear(); }
	virtual void clear() { }

	// 取得
	virtual label_t getLabel() const { return nullptr; }
	virtual int getAxCmd() const { return 0; }

	virtual int getUsing() const { return 0; }			// 使用状況 (0: 無効, 1: 有効, 2: クローン)
	virtual CPrmInfo const& getPrmInfo() const = 0;		// 仮引数

	// キャスト
	template<typename T>       T     castTo()       { return dynamic_cast<T>(this); }
	template<typename T> const T     castTo() const { return dynamic_cast<T>(this); }
	template<typename T>       T safeCastTo()       { return safeCastTo_Impl<T>(); }
	template<typename T> const T safeCastTo() const { return safeCastTo_Impl<T const>(); }

	template<typename T> T safeCastTo_Impl() const {
		auto const result = castTo<T>();
		if ( !result ) puterror(HSPERR_TYPE_MISMATCH);
		return result;
	}

	// 動作
	virtual void call( CCaller& caller ) = 0;

	// 形式的比較
	virtual int compare(IFunctor const& obj) const { return this - &obj; }
};

#endif
