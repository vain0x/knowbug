// Caller - header

#ifndef IG_CLASS_CALLER_H
#define IG_CLASS_CALLER_H

/**
@summary:
	call 処理を行う際に必須のクラス。
	実際の call 処理を行うためのインターフェース。

@role:
	- 中間コードや ctx への操作は CCaller が行う。
	- 実引数の処理。
	- call-jump 処理。
		1. prmstk の書き換え。
		2. gosub 処理。
		3. 返値の受け取り。

@impl:
	- 実際のデータ管理は、所有する CCall にすべて任せる。
		call データへのアクセスも、CCall* へのアクセスによって行うようにする。
	- 再入可能性はない。
**/

#include <stack>
#include <memory>
#include "hsp3plugin_custom.h"

#include "CCall.h"

using namespace hpimod;

class CFunctor;

extern CCall* TopCallStack();

class CCaller
{
public:
	enum class CallMode {
		Proc = 0,
		Bind,
	};

private:
	CallMode mMode;
	std::unique_ptr<CCall> mpCall;

	// caller 死亡後に使うため static メンバにしておく
	static PVal* stt_respval;

public:
	CCaller();
	CCaller( CallMode mode );

	// 動作
	void call();

	void setFunctor();
	void setFunctor( CFunctor const& functor );

	void setArgAll();
	bool setArgNext();
	void addArgByVal( void const* val, vartype_t vt );
	void addArgByRef( PVal* pval );
	void addArgByRef( PVal* pval, APTR aptr );
	void addArgByVarCopy( PVal* pval );

	// 取得
	CCall& getCall() const { return *mpCall; }

	PVal* getRetVal() const;
	vartype_t getCallResult(void** ppResult);

	static PVal* getLastRetVal();
	static void releaseLastRetVal();

private:
	CCaller( CCaller const& ) = delete;
	CCaller& operator = ( CCaller const& ) = delete;
};

#endif
