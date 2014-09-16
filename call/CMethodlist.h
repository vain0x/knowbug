// Method - CMethod のリスト

#ifndef IG_CLASS_METHOD_LIST_H
#define IG_CLASS_METHOD_LIST_H

/**
@summary:
	型と CMethod の関連づけを管理するクラス。
	map のラッパ。
@role:
	1. CMethod の生成(new) (および解体(delete))。
	2. CMethod へのアクセス 。
		CMethodlist を介さずに CMethod への参照を取得することはできない。
	3. 型への CMethod の割り付け。
**/

#include "CMethod.h"

class CMethodlist
{
private:
	using value_t = std::map<int, CMethod*>;

	//############################################
	//    メンバ変数
	//############################################
private:
	value_t* mplist;

	//############################################
	//    メンバ関数
	//############################################
public:
	//--------------------------------------------
	// 構築
	//--------------------------------------------
	CMethodlist()
		: mplist( new value_t )
	{ }

	//--------------------------------------------
	// 解体
	//--------------------------------------------
	~CMethodlist()
	{
		dbgout("~CMethodlist");
		for ( auto& it : *mplist ) {
			delete it.second;
		}
		delete mplist; mplist = nullptr;
		return;
	}

	//--------------------------------------------
	// CMethod をセット
	//--------------------------------------------
	void set( int vt )
	{
		if ( mplist->count(vt) > 0 ) {			// すでにメソッドが存在する
			return;
			/*
			value_t::iterator iter
				= mplist->find(vt);

			// 解放＋削除
			delete iter->second; iter->second = nullptr;
			mplist->erase( iter );
			//*/
		}

		CMethod* pMethod = new CMethod(vt);
		mplist->insert( value_t::value_type(vt, pMethod) );
		return;
	}

	//--------------------------------------------
	// CMethod を取り出す
	//--------------------------------------------
	CMethod* get( int vt ) const
	{
		auto iter = mplist->find(vt);
		return ( iter != mplist->end() )
			? iter->second
			: nullptr;
	}

//	operator std::map<int, CMethod*>*() { return mplist; }
};

#endif
