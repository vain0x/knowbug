// Call(ModCls) - FlexValue

// modcls により乗っ取られた struct 型の実体
// 互換性のため、構造体自体は hsp3struct.h によって定義されているものを流用する。

// あまり安全な設計ではないので、所有関係に注意して使うように……。

#ifndef IG_MODCLS_FLEX_VALUE_H
#define IG_MODCLS_FLEX_VALUE_H

#include "hsp3struct.h"

extern FlexValue* code_get_modinst();
extern FlexValue* code_get_modinst( FlexValue* def );

extern void FlexValue_AddRef   ( FlexValue const& self );
extern void FlexValue_Release  ( FlexValue const& self );		// デストラクタを呼ぶかもしれないことに注意
extern void FlexValue_DelRef   ( FlexValue& self );
extern void FlexValue_NullClear( FlexValue& self );

extern bool FlexValue_IsNull ( FlexValue const& self );
extern int  FlexValue_Counter( FlexValue const& self );
extern int  FlexValue_SubId  ( FlexValue const& self );
extern stdat_t     FlexValue_ModCls( FlexValue const& self );
extern char const* FlexValue_ClsName( FlexValue const& self );

extern void FlexValue_CtorWoCtorCalling( FlexValue& self, stdat_t modcls );
extern void FlexValue_Ctor( FlexValue& self, stdat_t modcls );
extern void FlexValue_Ctor( FlexValue& self, stdat_t modcls, PVal* pval, APTR aptr );
extern void FlexValue_Dtor( FlexValue& self );

extern void FlexValue_Copy( FlexValue& dst, FlexValue const& src );
extern void FlexValue_Move( FlexValue& dst, FlexValue& src );

extern void FlexValue_AddRefTmp ( FlexValue const& self );
extern void FlexValue_ReleaseTmp( FlexValue const& self );
extern bool FlexValue_IsTmp( FlexValue const& self );

extern bool FlexValueEx_Valid( FlexValue const& self );

static char const* ModCls_Name( stdat_t modcls )
{
	return ( modcls ? hpimod::STRUCTDAT_getName(modcls) : "#nullmod" );
}

int const FLEXVAL_COUNTER_DTORING = (-1);	// デストラクタ起動中

//------------------------------------------------
// FlexValue ホルダー
//------------------------------------------------
class FlexValueHolder
{
	FlexValue& mOwner;
	FlexValue mFv;

public:
	FlexValueHolder( FlexValue& fv )
		: mOwner(fv), mFv(fv)
	{
		FlexValue_AddRef( mFv );
	}
	~FlexValueHolder()
	{
		FlexValue_Move( mOwner, mFv );
	}

	FlexValue const& get() const { return mFv; }
	FlexValue& get() { return mFv; }
	operator FlexValue const&() const { return mFv; }

	      FlexValue* operator &()       { return &mFv; }
	FlexValue const* operator &() const { return &mFv; }

private:
	FlexValueHolder& operator = ( FlexValueHolder const& );
};

#endif
