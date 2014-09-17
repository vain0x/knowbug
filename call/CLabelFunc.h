// ラベル関数

#ifndef IG_CLASS_LABEL_FUNC_H
#define IG_CLASS_LABEL_FUNC_H

#include "hsp3plugin_custom.h"

#include "axcmd.h"
#include "IFunctor.h"
#include "CCaller.h"

#include "cmd_sub.h"

using namespace hpimod;

class CLabelFunc
	: public IFunctor
{
	label_t lb_;

public:
	CLabelFunc(label_t lb) : lb_ { lb } {}
	CLabelFunc(int axcmd_) {
		assert(AxCmd::isOk(axcmd_));
		switch ( AxCmd::getType(axcmd_) ) {
			case TYPE_LABEL:  lb_ = hpimod::getLabel(AxCmd::getCode(axcmd_)); break;
			case TYPE_MODCMD: lb_ = hpimod::getLabel(getSTRUCTDAT(AxCmd::getCode(axcmd_))->otindex); break;
		}
		puterror(HSPERR_TYPE_MISMATCH);
	}

	// 取得
	label_t getLabel() const override { return lb_; }
	int getAxCmd() const override { return AxCmd::make(TYPE_LABEL, hpimod::getOTPtr(lb_)); }

	int getUsing() const override { return HspBool(lb_ != nullptr); }			// 使用状況 (0: 無効, 1: 有効, 2: クローン)
	CPrmInfo const& getPrmInfo() const override {
		return GetPrmInfo(lb_);
	}

	// キャスト (IFunctor 用)
	/*
	template<typename T>       T     castTo()       { return dynamic_cast<T>(this); }
	template<typename T> const T     castTo() const { return dynamic_cast<T>(this); }
	template<typename T>       T safeCastTo()       { return safeCastTo_Impl<T>(); }
	template<typename T> const T safeCastTo() const { return safeCastTo_Impl<const T>(); }
	//*/

	// 動作
	void call(CCaller& caller) override {
		return caller.getCall().callLabel(getLabel());
	}

	// 形式的比較
};

#endif
