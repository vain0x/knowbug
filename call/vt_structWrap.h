// Call(ModCls) - vartype(struct)

#ifndef IG_VARTYPE_STRUCT_WRAP_H
#define IG_VARTYPE_STRUCT_WRAP_H

#include <map>
#include "hsp3plugin_custom.h"

using namespace hpimod;
class CFunctor;

// HspVarProc(struct) の処理関数はすべて置き換える。
// @ HSP本体との互換性は維持する。
// @ FlexValue::ptr (メンババッファ) は、参照カウンタのために余分に 8[byte] 確保する。
// @ -	メンバおよびメンババッファは参照カウンタが 0 になったときに初めて破棄する。
// @ -	メンババッファが解放される場合、それを共有していた全てのインスタンスを nullptr にする。
// @	参照カウンタが 0 なら、参照カウンタを得るためにメンババッファを参照するが、それは解放されているので、落ちる。
// @ FlexValue::clonetype は全て 1 にして、終了時の自動的デストラクトを行わない。
// @ -	ただし、スクリプト側で、clonetype == 0 であるインスタンス(termer)を1つだけ生成し、
// @	それの自動起動デストラクタで、ModOp_Term() を呼ぶ。そこで全ての生存インスタンスを始末する。

// @ × mpval はクローン変数としてのみ動作する
// @	最後に解体されるとき、mpval が参照を持っていたら、テンポラリ変数のない状態でデストラクタを呼ぶことになるため。
// @	( mpval に直接参照できたなら、ModOp_Term() で解体できるので問題ないが…… )
// @→ 直接参照できるようにしたので mpval も普通に modinst 型変数として機能する。

extern void HspVarStructWrap_Init( HspVarProc* vp );
extern void HspVarStructWrap_Dup( FlexValue* result, FlexValue* fv );

namespace ModCls {

using OpFuncDefs   = std::map<int, CFunctor>;	// 演算関数の連想配列 (添字: OpId_*)
using CModOperator = std::map<int, OpFuncDefs>;	// (モジュールクラス, 演算関数)

// クラスと演算関数の対応を保持するクラス
extern CModOperator const* getModOperator();

// テンポラリ変数の参照
extern PVal* getMPValStruct();
extern PVal* getMPVal(vartype_t type);

// nullmod 変数
extern PVal const* getNullmod();

namespace VtStruct
{
	using value_type = FlexValue*;
	using valptr_t = FlexValue*;
	using const_valptr_t = FlexValue const*;

	inline const_valptr_t asValptr(void const* pdat) { return reinterpret_cast<const_valptr_t>(pdat); }
	inline valptr_t asValptr(void* pdat) { return const_cast<valptr_t>(asValptr(static_cast<void const*>(pdat))); }

}

}

#endif
