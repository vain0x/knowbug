// 関数子オブジェクト

// 一般に、「関数」として呼び出せるものを扱うためのオブジェクト。
// 基本関数子 { ラベル、ユーザ定義関数 } か、特殊関数子 (IFunctorEx) を持つ。
// それらの具体的な違いを緩衝するためのインターフェースである。

// 所持するメンバ変数はすべて単純コピー可能なものであり、サイズも小さいため、
// 参照ではなく実体として使用してよい。

#ifndef IG_CLASS_FUNCTOR_H
#define IG_CLASS_FUNCTOR_H

#include "hsp3plugin_custom.h"
#include "axcmd.h"

using namespace hpimod;

class CPrmInfo;
class CCaller;

class CFunctor;
typedef       CFunctor functor_t;
typedef const CFunctor cfunctor_t;

class IFunctorEx;
using exfunctor_t = IFunctorEx*;

//################################################
//    定義部
//################################################
//------------------------------------------------
// 関数子タイプ
//------------------------------------------------
enum FuncType
{
	FuncType_None = 0,	// なし (無効)
	FuncType_Label,		// ラベル
	FuncType_Deffid,	// ユーザ定義関数
	FuncType_Ex,		// 特殊関数子
	FuncType_MAX
};

//------------------------------------------------
// 関数子を表すクラス
//------------------------------------------------
class CFunctor
{
private:
	FuncType type;
	union {
		label_t  lb;		// ラベル関数
		int      deffid;	// ユーザ定義関数
		exfunctor_t ex;		// 特殊関数子
	};

public:
	CFunctor() : type( FuncType_None ) { }
	CFunctor( label_t  _lb    ) : type( FuncType_Label ), lb( _lb ) { }
	CFunctor( int      _axcmd );
	CFunctor( exfunctor_t _ex );

	CFunctor( CFunctor const& src )
		: type( FuncType_None )
	{ this->copy( src ); }

	~CFunctor();

	void clear();

	// 取得
	FuncType getType() const { return type; }

	int             getUsing()   const;	// 使用状況 (0: 無効, 1: 有効, 2: クローン)
	CPrmInfo const& getPrmInfo() const;	// not nullptr

	label_t   getLabel() const;		// or nullptr
	int       getAxCmd() const;		// or 0
	exfunctor_t getEx()  const { return getType() == FuncType_Ex ? ex : nullptr; }

	// キャスト (IFunctorEx 用)
	template<class T>       T     castTo()       { return dynamic_cast<T>( getEx() ); }
	template<class T> const T     castTo() const { return dynamic_cast<T>( getEx() ); }
	template<class T>       T safeCastTo()       { return safeCastTo_Impl<T>(); }
	template<class T> const T safeCastTo() const { return safeCastTo_Impl<const T>(); }

	template<class T> T safeCastTo_Impl() const {
		auto result = castTo<T>();
		if ( !result ) puterror( HSPERR_TYPE_MISMATCH );
		return result;
	}

	// 動作
	void call( CCaller& caller );

	// 演算
	CFunctor& operator =( CFunctor const& src ) { return this->copy( src ); }

	int compare( CFunctor const& src ) const;

private:
	CFunctor& copy( CFunctor const& );

};

#endif
