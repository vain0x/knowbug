// ユーザ定義コマンドの関数

#ifndef IG_CLASS_DEFFUNC_H
#define IG_CLASS_DEFFUNC_H

#include "hsp3plugin_custom.h"

#include "axcmd.h"
#include "IFunctor.h"
#include "CCaller.h"

#include "cmd_sub.h"

using namespace hpimod;

class CDeffunc
	: public IFunctor
{
	int deffid_;

public:
	CDeffunc(int axcmd_) {
		assert(AxCmd::isOk(axcmd_));
		if ( AxCmd::getType(axcmd_) == TYPE_MODCMD ) {
			deffid_ = AxCmd::getCode(axcmd_);
		} else {
			puterror(HSPERR_TYPE_MISMATCH);
		}
	}

	// 取得
	label_t getLabel() const override { return hpimod::getLabel(getSTRUCTDAT(deffid_)->otindex); }
	int getAxCmd() const override { return AxCmd::make(TYPE_MODCMD, deffid_); }

	int getUsing() const override { return 1; }
	CPrmInfo const& getPrmInfo() const override {
		return GetPrmInfo( getSTRUCTDAT(deffid_) );
	}

	// キャスト (IFunctor 用)
	/*
	template<typename T>       T     castTo()       { return dynamic_cast<T>(this); }
	template<typename T> const T     castTo() const { return dynamic_cast<T>(this); }
	template<typename T>       T safeCastTo()       { return safeCastTo_Impl<T>(); }
	template<typename T> const T safeCastTo() const { return safeCastTo_Impl<const T>(); }
	//*/

	// 動作
	virtual void call(CCaller& caller) override {
		return caller.getCall().callLabel(getLabel());
	}

	// 形式的比較
};

#endif
