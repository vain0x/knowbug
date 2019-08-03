#pragma once

#include "hsx_data.h"
#include "hsx_dim_index.h"
#include "hsx_param_data.h"
#include "hsx_param_stack.h"
#include "hsx_slice.h"
#include "hsx_types_fwd.h"
#include "hsx_var_metadata.h"
#include "memory_view.h"

namespace hsp_sdk_ext {
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

	extern auto element_to_indexes(PVal const* pval, std::size_t aptr)->std::optional<HspDimIndex>;

	extern auto element_to_aptr(PVal const* pval, HspDimIndex const& indexes)->std::optional<std::size_t>;

	extern auto element_to_data(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->std::optional<HspData>;

	extern auto element_to_memory_block(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->MemoryView;

	extern auto element_to_str(PVal const* pval, std::size_t aptr, HSPCTX const* ctx)->std::optional<HspStr>;

	extern auto flex_is_nullmod(FlexValue const* flex) -> bool;

	extern auto flex_is_clone(FlexValue const* flex) -> bool;

	extern auto flex_to_struct(FlexValue const* flex, HSPCTX const* ctx)->std::optional<STRUCTDAT const*>;

	extern auto flex_to_member_count(FlexValue const* flex, HSPCTX const* ctx)->std::size_t;

	extern auto flex_to_member(FlexValue const* flex, std::size_t member_index, HSPCTX const* ctx)->std::optional<HspParamData>;

	extern auto flex_to_param_stack(FlexValue const* flex, HSPCTX const* ctx)->std::optional<HspParamStack>;

	extern auto mp_var_to_pval(MPVarData const* mp_var)->PVal const*;

	extern auto mp_var_to_aptr(MPVarData const* mp_var)->std::size_t;

	extern auto mp_mod_var_to_pval(MPModVarData const* mp_mod_var)->PVal const*;

	extern auto mp_mod_var_to_aptr(MPModVarData const* mp_mod_var)->std::size_t;

	extern auto object_temps(HSPCTX const* ctx)->Slice<HspObjectTemp>;

	extern auto object_temp_count(HSPCTX const* ctx)->std::size_t;

	extern auto object_temp_to_label(std::size_t ot_index, HSPCTX const* ctx)->std::optional<HspLabel>;

	extern auto params(HSPCTX const* ctx)->Slice<STRUCTPRM>;

	extern auto param_to_type(STRUCTPRM const* param)->HspParamType;

	extern auto param_to_struct(STRUCTPRM const* param, HSPCTX const* ctx)->std::optional<STRUCTDAT const*>;

	extern auto param_data_to_type(HspParamData const& param_data)->HspParamType;

	extern auto param_data_to_pval(HspParamData const& param_data)->std::optional<PVal const*>;

	extern auto param_data_to_mp_var(HspParamData const& param_data)->std::optional<MPVarData const*>;

	extern auto param_data_to_mp_mod_var(HspParamData const& param_data)->std::optional<MPModVarData const*>;

	extern auto param_data_to_mp_mod_var(HspParamType param_type, void const* data)->std::optional<MPModVarData const*>;

	extern auto param_data_to_data(HspParamData const& param_data)->std::optional<HspData>;

	extern auto param_data_to_str(HspParamData const& param_data)->std::optional<HspStr>;

	extern auto param_stack_to_param_data_count(HspParamStack const& param_stack)->std::size_t;

	extern auto param_stack_to_param_data(HspParamStack const& param_stack, std::size_t param_index, HSPCTX const* ctx)->std::optional<HspParamData>;

	extern auto param_type_to_name(HspParamType param_type)->std::optional<char const*>;

	extern auto pval_to_type(PVal const* pval)->HspType;

	extern auto pval_to_varmode(PVal const* pval)->HspVarMode;

	extern auto pval_to_lengths(PVal const* pval)->HspDimIndex;

	extern auto pval_to_element_count(PVal const* pval)->std::size_t;

	extern auto pval_is_standard_array(PVal const* pval, HSPCTX const* ctx)->bool;

	extern auto pval_to_data(PVal const* pval, HSPCTX const* ctx)->std::optional<HspData>;

	extern auto pval_to_memory_block(PVal const* pval, HSPCTX const* ctx)->MemoryView;

	extern auto static_vars(HSPCTX const* ctx)->Slice<PVal>;

	extern auto static_var_count(HSPCTX const* ctx)->std::size_t;

	extern auto static_var_to_pval(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<PVal const*>;

	extern auto static_var_from_name(char const* var_name, HSPCTX const* ctx)->std::optional<std::size_t>;

	extern auto static_var_to_name(std::size_t static_var_index, HSPCTX const* ctx)->std::optional<char const*>;

	extern auto structs(HSPCTX const* ctx)->Slice<STRUCTDAT>;

	extern auto struct_to_name(STRUCTDAT const* struct_dat, HSPCTX const* ctx)->std::optional<char const*>;

	extern auto struct_to_param_count(STRUCTDAT const* struct_dat)->std::size_t;

	extern auto struct_to_params(STRUCTDAT const* struct_dat, HSPCTX const* ctx)->Slice<STRUCTPRM>;

	extern auto struct_to_param_stack_size(STRUCTDAT const* struct_dat)->std::size_t;

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

	// 実行位置を更新する。
	extern void debug_do_update_location(HSP3DEBUG* debug);

	// デバッグモード (HSPDEBUG_*) を設定する。
	extern auto debug_do_set_mode(int mode, HSP3DEBUG* debug) -> bool;

	// 実行位置のファイル参照名 (`#include` に指定されたパス)
	extern auto debug_to_file_ref_name(HSP3DEBUG const* debug)->std::optional<char const*>;

	// 実行位置の行番号 (0-indexed) を取得する。
	extern auto debug_to_line_index(HSP3DEBUG const* debug)->std::size_t;

	// 全般の情報。
	// フォーマット: `key1\nvalue1\nkey2\nvalue2\n..`
	extern auto debug_to_general_info(HSP3DEBUG* debug)->std::unique_ptr<char, void(*)(char*)>;

	// 静的変数の名前のリスト (改行区切り)
	extern auto debug_to_static_var_names(HSP3DEBUG* debug)->std::unique_ptr<char, void(*)(char*)>;
}
