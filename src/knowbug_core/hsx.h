// HSP SDK Extensions (hsx)
//
// 目的: HSP SDK の安全かつ抽象的な API を提供する。
//
// 理由: HSP ランタイムのデータ構造は性能や整合性のために設計されていて、
//      代わりに安全性や抽象度が低いため。
// (NULL 検査や範囲検査などがなくて安全でなく、ほしい情報のありかなどが抽象的でない。)

#pragma once

#include "hsx_data.h"
#include "hsx_dim_index.h"
#include "hsx_param_data.h"
#include "hsx_param_stack.h"
#include "hsx_slice.h"
#include "hsx_types_fwd.h"
#include "hsx_var_metadata.h"
#include "memory_view.h"

namespace hsx {
	// HSP の文字列データ。
	// 1. str 型はランタイムエンコーディング (shift_jis/utf-8) の文字列だけでなく、
	// 他のエンコーディングの文字列や任意のバイナリを格納するのにも使われることがたまにある。
	// 特に、null 終端とは限らない点に注意。std::strlen などの null 終端を前提とする関数に渡してはいけない。
	// 2. 変数や refstr に由来する文字列データはバッファサイズが容易に取得できる。
	// str 引数の文字列データはバッファサイズを取得できないが、null 終端が保証されている。
	using HspStr = Slice<char>;

	extern auto data_from_label(HspLabel const* ptr)->HspData;

	extern auto data_from_str(char const* ptr)->HspData;

	extern auto data_from_double(HspDouble const* ptr)->HspData;

	extern auto data_from_int(HspInt const* ptr)->HspData;

	extern auto data_from_flex(FlexValue const* ptr)->HspData;

	extern auto data_to_label(HspData const& data)->std::optional<HspLabel>;

	extern auto data_to_str(HspData const& data)->std::optional<char const*>;

	extern auto data_to_double(HspData const& data)->std::optional<HspDouble>;

	extern auto data_to_int(HspData const& data)->std::optional<HspInt>;

	extern auto data_to_flex(HspData const& data)->std::optional<FlexValue const*>;

	// APTR が指している要素に対応する、配列の添字を計算する。
	// 範囲外を指しているときは nullopt。
	extern auto element_to_indexes(PVal const* pval, std::size_t aptr)->std::optional<HspDimIndex>;

	// 配列の指定した要素を指す APTR を計算する。
	// 範囲外を指しているときは nullopt。
	extern auto element_to_aptr(PVal const* pval, HspDimIndex const& indexes)->std::optional<std::size_t>;

	// 配列要素の実体ポインタを得る。
	// FIXME: 配列の書き換えを伴うので、const を外した方がいい？
	extern auto element_to_data(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->std::optional<HspData>;

	// 配列要素のメモリブロックを得る。
	// 範囲外のときは空のメモリブロックが返る。
	extern auto element_to_memory_block(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->MemoryView;

	// 配列要素が文字列型なら、そのデータへの参照を得る。
	extern auto element_to_str(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->std::optional<HspStr>;

	// フレックスが null か？
	// dimtype で作られた newmod されていない要素や、delmod された要素は null である。
	extern auto flex_is_nullmod(FlexValue const* flex) -> bool;

	// フレックスが clone 状態か？
	// 代入で作られた要素は clone 状態である。
	// FIXME: dup/dupptr で作られた要素は clone 状態ではなかったはずなので、確認して、その旨を注記する。
	extern auto flex_is_clone(FlexValue const* flex) -> bool;

	// フレックスの基になるモジュールの情報を得る。
	// フレックスが null なら nullopt。
	extern auto flex_to_struct(FlexValue const* flex, HSPCTX const* ctx)->std::optional<STRUCTDAT const*>;

	// フレックスのメンバ変数の数を得る。
	// フレックスが null なら 0。
	extern auto flex_to_member_count(FlexValue const* flex, HSPCTX const* ctx)->std::size_t;

	// FIXME: ローカル変数なので PVal* を返す方がよいかもしれない
	extern auto flex_to_member(FlexValue const* flex, std::size_t member_index, HSPCTX const* ctx)->std::optional<HspParamData>;

	extern auto flex_to_param_stack(FlexValue const* flex, HSPCTX const* ctx)->std::optional<HspParamStack>;

	// var/array 引数に渡された配列要素の PVal を得る。
	extern auto mp_var_to_pval(MPVarData const* mp_var)->PVal const*;

	// var/array 引数に渡された配列要素の APTR を得る。
	// 注意: APTR は範囲外を指す可能性がある。(配列が dim などでリセットされる可能性があるため。)
	extern auto mp_var_to_aptr(MPVarData const* mp_var)->std::size_t;

	// thismod 引数に渡された配列要素の PVal を得る。
	extern auto mp_mod_var_to_pval(MPModVarData const* mp_mod_var)->PVal const*;

	// thismod 引数に渡された配列要素の APTR を得る。
	// 注意: APTR は範囲外を指す可能性がある。
	extern auto mp_mod_var_to_aptr(MPModVarData const* mp_mod_var)->std::size_t;

	extern auto object_temps(HSPCTX const* ctx)->Slice<HsxObjectTemp>;

	// オブジェクトテンポラリ領域に格納されているラベルの個数を得る。
	extern auto object_temp_count(HSPCTX const* ctx)->std::size_t;

	// オブジェクトテンポラリ領域の要素からラベル値を生成する。
	extern auto object_temp_to_label(std::size_t ot_index, HSPCTX const* ctx)->std::optional<HspLabel>;

	extern auto params(HSPCTX const* ctx)->Slice<STRUCTPRM>;

	extern auto param_to_type(STRUCTPRM const* param)->HspParamType;

	// この引数を所有する struct を得る。
	extern auto param_to_struct(STRUCTPRM const* param, HSPCTX const* ctx)->std::optional<STRUCTDAT const*>;

	extern auto param_data_to_type(HspParamData const& param_data)->HspParamType;

	// 引数データが配列や配列要素を指しているなら、その PVal を得る。
	extern auto param_data_to_pval(HspParamData const& param_data)->std::optional<PVal const*>;

	// var/array 引数の詳細を得る。
	extern auto param_data_to_mp_var(HspParamData const& param_data)->std::optional<MPVarData const*>;

	// thismod 引数の詳細を得る。
	extern auto param_data_to_mp_mod_var(HspParamData const& param_data)->std::optional<MPModVarData const*>;

	// thismod 引数の詳細を得る。
	extern auto param_data_to_mp_mod_var(HspParamType param_type, void const* data)->std::optional<MPModVarData const*>;

	// 引数データが指している配列の先頭要素や、配列要素、リテラルから、データの実体ポインタを得る。
	extern auto param_data_to_data(HspParamData const& param_data)->std::optional<HspData>;

	// 引数データが指している文字列データを得る。
	extern auto param_data_to_str(HspParamData const& param_data)->std::optional<HspStr>;

	// 引数スタックに含まれる引数データの個数を得る。
	extern auto param_stack_to_param_data_count(HspParamStack const& param_stack)->std::size_t;

	extern auto param_stack_to_param_data(HspParamStack const& param_stack, std::size_t param_index, HSPCTX const* ctx)->std::optional<HspParamData>;

	// 引数タイプの名称を得る。(一部のみ。sptr など、#func のものは未対応。)
	extern auto param_type_to_name(HspParamType param_type)->std::optional<char const*>;

	extern auto pval_to_type(PVal const* pval)->HspType;

	extern auto pval_to_varmode(PVal const* pval)->HspVarMode;

	// 配列の各次元の長さを得る。
	extern auto pval_to_lengths(PVal const* pval)->HspDimIndex;

	// 配列の総要素数を得る。
	extern auto pval_to_element_count(PVal const* pval)->std::size_t;

	// 配列が「標準的な配列」か？
	extern auto pval_is_standard_array(PVal const* pval, HSPCTX const* ctx)->bool;

	// 配列の先頭要素の実体ポインタを得る。
	extern auto pval_to_data(PVal const* pval, HSPCTX const* ctx)->std::optional<HspData>;

	// 配列の先頭要素のメモリブロックを得る。
	extern auto pval_to_memory_block(PVal const* pval, HSPCTX const* ctx)->MemoryView;

	// 配列が文字列型なら、そのデータへの参照を得る。
	extern auto pval_to_str(PVal const* pval, HSPCTX const* ctx)->std::optional<HspStr>;

	extern auto static_vars(HSPCTX const* ctx)->Slice<PVal>;

	extern auto static_var_count(HSPCTX const* ctx)->std::size_t;

	extern auto static_var_to_pval(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<PVal const*>;

	// 名前から静的変数を探す。
	extern auto static_var_from_name(char const* var_name, HSPCTX const* ctx)->std::optional<std::size_t>;

	// 静的変数の名前を得る。
	// デバッグモードなど、変数名が残る設定でビルドしていないと取得できない。
	extern auto static_var_to_name(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<char const*>;

	extern auto structs(HSPCTX const* ctx)->Slice<STRUCTDAT>;

	// struct に対応する名前を得る。
	extern auto struct_to_name(STRUCTDAT const* struct_dat, HSPCTX const* ctx)->std::optional<char const*>;

	// struct が所有する引数の数を得る。
	extern auto struct_to_param_count(STRUCTDAT const* struct_dat)->std::size_t;

	// struct が所有する引数の情報のリストを得る。
	extern auto struct_to_params(STRUCTDAT const* struct_dat, HSPCTX const* ctx)->Slice<STRUCTPRM>;

	// struct に対応する引数スタックのサイズを得る。
	// (配置されるデータによらず、引数スタックのサイズは一定である。)
	extern auto struct_to_param_stack_size(STRUCTDAT const* struct_dat)->std::size_t;

	// システム変数 `cnt` の実体ポインタを得る。
	extern auto system_var_cnt(HSPCTX const* ctx)->std::optional<HspInt const*>;

	extern auto system_var_err(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_iparam(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_wparam(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_lparam(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_looplev(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_sublev(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_refstr(HSPCTX const* ctx)->HspStr;

	extern auto system_var_refdval(HSPCTX const* ctx)->HspDouble const*;

	extern auto system_var_stat(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_strsize(HSPCTX const* ctx)->HspInt const*;

	extern auto system_var_thismod(HSPCTX const* ctx)->std::optional<MPModVarData const*>;

	extern auto system_var_to_data(HspSystemVarKind system_var_kind, HSPCTX const* ctx)->std::optional<HspData>;

	extern auto debug_to_context(HSP3DEBUG const* debug)->HSPCTX const*;

	// 実行位置に関する最新の情報を取りに行く。
	// データは `debug_to_file_ref_name` と `debug_to_line_index` で確認できる。
	extern void debug_do_update_location(HSP3DEBUG* debug);

	// デバッグモード (HSPDEBUG_*) を設定する。ステップ実行や中断・再開に使う。
	extern auto debug_do_set_mode(int mode, HSP3DEBUG* debug) -> bool;

	// 実行位置のファイル参照名 (`#include` に指定されたパス)。
	// FIXME: 取得したポインタがいつまで有効か調査
	extern auto debug_to_file_ref_name(HSP3DEBUG const* debug)->std::optional<char const*>;

	// 実行位置の行番号 (0-indexed) を取得する。
	extern auto debug_to_line_index(HSP3DEBUG const* debug)->std::size_t;

	// 全般の情報。
	// フォーマット: `key1\nvalue1\nkey2\nvalue2\n...\n` (キーと値が改行区切りで交互に出現する。)
	extern auto debug_to_general_info(HSP3DEBUG* debug)->std::unique_ptr<char, void(*)(char*)>;

	// 静的変数の名前のリスト。
	// フォーマット: `name1\nname2\n...\n` (改行区切り)
	extern auto debug_to_static_var_names(HSP3DEBUG* debug)->std::unique_ptr<char, void(*)(char*)>;
}
