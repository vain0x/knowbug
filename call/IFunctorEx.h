// 特殊関数子インターフェース

// 特殊な「関数」を表すために、プラグイン側で作成するクラスたちの基底クラス。
// CFunctor から exfuntor_t 型として参照されるために存在する。
// 例えば、CBound, CCoRoutine, CLambda などに継承されている。

// 参照カウンタとしての機能を持っている。

#ifndef IG_INTERFACE_FUNCTOR_EXTRA_H
#define IG_INTERFACE_FUNCTOR_EXTRA_H

#include "hsp3plugin_custom.h"
using namespace hpimod;

//	#define DBGOUT_FUNCTOR_EX_ADDREF_OR_RELEASE	// AddRef, Release を dbgout で報告する

class CPrmInfo;
class CCaller;
class CFunctor;

class IFunctorEx;
using exfunctor_t = IFunctorEx*;

//------------------------------------------------
// 特殊な関数子を表すクラス
// 
// @ 継承して使う。
// @ ここでは既定動作を定義している。
//------------------------------------------------
class IFunctorEx
{
protected:
	IFunctorEx();

public:
	virtual ~IFunctorEx();

	// 取得
	virtual label_t getLabel() const { return nullptr; }
	virtual int getAxCmd() const { return 0; }

	virtual int getUsing() const { return 0; }	// 使用状況 (0: 無効, 1: 有効, 2: クローン)
	virtual CPrmInfo const& getPrmInfo() const = 0;				// 仮引数

	// 動作
	virtual void call( CCaller& caller ) = 0;

	// その他
	virtual int compare( exfunctor_t const& obj ) const { return this - obj; }	// obj は常に this と同じかその派生の型

private:
	// 封印
	IFunctorEx( IFunctorEx const& ) = delete;
	IFunctorEx& operator =( IFunctorEx const& ) = delete;

	// 参照カウンタ関連
private:
	mutable int mRefer;

public:
#ifdef DBGOUT_FUNCTOR_EX_ADDREF_OR_RELEASE
	int mId;
	void AddRef()  const { mRefer ++; dbgout("[%d] ++ → %d", mId, mRefer); }
	bool Release() const { mRefer --; dbgout("[%d] -- → %d", mId, mRefer); return (mRefer <= 0); }
#else
	void AddRef()  const { mRefer ++; }
	bool Release() const { return ( (-- mRefer) <= 0 ); }
#endif

//	static exfunctor_t New();	// 生成関数
	static void Delete( exfunctor_t& self );

	static void ReleaseAllInstance();
};

inline void FunctorEx_AddRef( exfunctor_t self ) {
	if ( self ) self->AddRef();
	return;
}

inline void FunctorEx_Release( exfunctor_t& self ) {
	if ( self ) {
		if ( self->Release() ) IFunctorEx::Delete(self);
	}
	return;
}
#endif
