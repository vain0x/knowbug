// CMethod

#ifndef IG_CLASS_METHOD_H
#define IG_CLASS_METHOD_H

/**
@summary:
	型1つのメソッド群を管理する。
**/

#include <map>
#include <string>
#include "hsp3plugin_custom.h"

#include "CFunctor.h"
#include "cmd_call.h"

class CMethod
{
private:
	typedef std::map<std::string, CFunctor> methodlist_t;

	//############################################
	//    メンバ変数
	//############################################
private:
	vartype_t mVartype;
	methodlist_t* mpMethodlist;

	//############################################
	//    メンバ関数
	//############################################
public:
	explicit CMethod( vartype_t vt = HSPVAR_FLAG_NONE );
	virtual ~CMethod();

	int getVartype(void) const
	{
		return mVartype;
	}

	//--------------------------------------------
	// メソッドを追加
	//--------------------------------------------
	void add( const std::string& name, CFunctor const& functor );

	//--------------------------------------------
	// メソッドを呼び出す
	// @ 命令形式
	//--------------------------------------------
	void call( const std::string& name, PVal* pvThis );

	//--------------------------------------------
	// 
	//--------------------------------------------

private:
	CMethod( CMethod const& src );
	CMethod& operator = (CMethod const& src );

};

#endif
