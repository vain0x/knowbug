#include "pch.h"
#include "../hpiutil/hpiutil.hpp"
#include "../hpiutil/dinfo.hpp"
#include "HspDebugApi.h"

#undef max

#define UNSAFE(E) (E)

namespace hsx = hsp_sdk_ext;

HspDebugApi::HspDebugApi(HSP3DEBUG* debug)
	: debug_(debug)
	, context_(debug->hspctx)
	, exinfo_(debug->hspctx->exinfo2)
{
}

auto HspDebugApi::current_file_ref_name() const -> std::optional<char const*> {
	return hsx::debug_to_file_ref_name(debug());
}

auto HspDebugApi::current_line() const -> std::size_t {
	return hsx::debug_to_line_index(debug());
}

auto HspDebugApi::static_vars() const -> PVal const* {
	return hsx::static_vars(context()).data();
}

auto HspDebugApi::static_var_count() const -> std::size_t {
	return hsx::static_var_count(context());
}

auto HspDebugApi::static_var_find_by_name(char const* var_name) const -> std::optional<std::size_t> {
	return hsx::static_var_from_name(var_name, context());
}

auto HspDebugApi::static_var_find_name(std::size_t static_var_id) const -> std::optional<std::string> {
	return hsx::static_var_to_name(static_var_id, context());
}

auto HspDebugApi::static_var_to_pval(std::size_t static_var_id) const -> PVal const* {
	auto&& pval_opt = hsx::static_var_to_pval(static_var_id, context());
	if (!pval_opt) {
		assert(false && u8"invalid static_var_id");
		throw new std::exception{};
	}

	return *pval_opt;
}

auto HspDebugApi::var_to_type(PVal const* pval) const -> HspType {
	return hsx::pval_to_type(pval);
}

auto HspDebugApi::var_to_data(PVal const* pval) const -> HspData {
	return hsx::pval_to_data(pval, context()).value_or(HspData{});
}

auto HspDebugApi::var_to_lengths(PVal const* pval) const -> HspDimIndex {
	return hsx::pval_to_lengths(pval);
}

auto HspDebugApi::var_to_mode(PVal const* pval) const -> HspVarMode {
	return hsx::pval_to_varmode(pval);
}

bool HspDebugApi::var_is_array(PVal const* pval) const {
	return hsx::pval_is_standard_array(pval, context());
}

auto HspDebugApi::var_to_element_count(PVal const* pval) const -> std::size_t {
	return hsx::pval_to_element_count(pval);
}

auto HspDebugApi::var_element_to_indexes(PVal const* pval, std::size_t aptr) const -> std::optional<HspDimIndex> {
	return hsx::element_to_indexes(pval, aptr);
}

auto HspDebugApi::var_element_to_aptr(PVal const* pval, HspDimIndex const& indexes) const -> std::optional<std::size_t> {
	return hsx::element_to_aptr(pval, indexes);
}

auto HspDebugApi::var_element_to_data(PVal const* pval, std::size_t aptr) const -> HspData {
	return hsx::element_to_data(pval, aptr, context()).value_or(HspData{});
}

auto HspDebugApi::var_to_block_memory(PVal const* pval) const -> MemoryView {
	return hsx::pval_to_memory_block(pval, context());
}

auto HspDebugApi::var_element_to_block_memory(PVal const* pval, std::size_t aptr) const -> MemoryView {
	return hsx::element_to_memory_block(pval, aptr, context());
}

auto HspDebugApi::system_var_to_data(HspSystemVarKind system_var_kind) const -> std::optional<HspData> {
	return hsx::system_var_to_data(system_var_kind, context());
}

auto HspDebugApi::data_to_label(HspData const& data) const -> HspLabel {
	auto&& opt = hsx::data_to_label(data);
	if (!opt) {
		assert(false && u8"Invalid cast to label");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::data_to_str(HspData const& data) const -> HspStr {
	auto&& opt = hsx::data_to_str(data);
	if (!opt) {
		assert(false && u8"Invalid cast to string");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::data_to_double(HspData const& data) const -> HspDouble {
	auto&& opt = hsx::data_to_double(data);
	if (!opt) {
		assert(false && u8"Invalid cast to double");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::data_to_int(HspData const& data) const -> HspInt {
	auto&& opt = hsx::data_to_int(data);
	if (!opt) {
		assert(false && u8"Invalid cast to int");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::data_to_flex(HspData const& data) const -> FlexValue const* {
	auto&& opt = hsx::data_to_flex(data);
	if (!opt) {
		assert(false && u8"Invalid cast to struct");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::static_label_count() const -> std::size_t {
	return hsx::object_temp_count(context());
}

auto HspDebugApi::static_label_to_label(std::size_t static_label_id) const -> std::optional<HspLabel> {
	return hsx::object_temp_to_label(static_label_id, context());
}

bool HspDebugApi::flex_is_nullmod(FlexValue const* flex) const {
	return hsx::flex_is_clone(flex);
}

bool HspDebugApi::flex_is_clone(FlexValue const* flex) const {
	return hsx::flex_is_clone(flex);
}

auto HspDebugApi::flex_to_module_struct(FlexValue const* flex) const -> STRUCTDAT const* {
	return *hsx::flex_to_struct(flex, context());
}

auto HspDebugApi::flex_to_member_count(FlexValue const* flex) const -> std::size_t {
	return hsx::flex_to_member_count(flex, context());
}

auto HspDebugApi::flex_to_member_at(FlexValue const* flex, std::size_t member_index) const -> HspParamData {
	return *hsx::flex_to_member(flex, member_index, context());
}

auto HspDebugApi::flex_to_param_stack(FlexValue const* flex) const -> HspParamStack {
	return *hsx::flex_to_param_stack(flex, context());
}

auto HspDebugApi::struct_to_name(STRUCTDAT const* struct_dat) const -> char const* {
	return hsx::struct_to_name(struct_dat, context()).value_or(u8"");
}

auto HspDebugApi::struct_to_param_count(STRUCTDAT const* struct_dat) const -> std::size_t {
	return hsx::struct_to_param_count(struct_dat);
}

auto HspDebugApi::struct_to_param_at(STRUCTDAT const* struct_dat, std::size_t param_index) const -> std::optional<STRUCTPRM const*> {
	return hsx::struct_to_params(struct_dat, context()).get(param_index);
}

auto HspDebugApi::struct_to_param_stack_size(STRUCTDAT const* struct_dat) const -> std::size_t {
	return hsx::struct_to_param_stack_size(struct_dat);
}

auto HspDebugApi::params() const -> STRUCTPRM const* {
	return hpiutil::minfo().begin();
}

auto HspDebugApi::param_count() const -> std::size_t {
	return hpiutil::minfo().size();
}

auto HspDebugApi::param_to_param_id(STRUCTPRM const* param) const -> std::size_t {
	auto id = hpiutil::STRUCTPRM_miIndex(param);
	if (id < 0) {
		assert(false && u8"Invalid STRUCTPRM");
		throw new std::exception{};
	}

	return (std::size_t)id;
}

auto HspDebugApi::param_to_name(STRUCTPRM const* param, std::size_t param_index, hpiutil::DInfo const& debug_segment) const -> std::string {
	return hpiutil::nameFromStPrm(param, (int)param_index, debug_segment);
}

auto HspDebugApi::param_stack_to_data_count(HspParamStack const& param_stack) const -> std::size_t {
	return hsx::struct_to_param_count(param_stack.struct_dat());
}

auto HspDebugApi::param_stack_to_data_at(HspParamStack const& param_stack, std::size_t param_index) const -> std::optional<HspParamData> {
	return hsx::param_stack_to_param_data(param_stack, param_index, context());
}

auto HspDebugApi::param_type_to_name(HspParamType param_type) const -> char const* {
	return hsx::param_type_to_name(param_type).value_or(u8"???");
}

auto HspDebugApi::param_data_to_type(HspParamData const& param_data) const -> HspParamType {
	return hsx::param_data_to_type(param_data);
}

auto HspDebugApi::param_data_as_local_var(HspParamData const& param_data) const -> PVal const* {
	auto&& opt = hsx::param_data_to_pval(param_data);
	if (!opt) {
		assert(false && u8"Casting to local var");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::param_data_to_single_var(HspParamData const& param_data) const -> MPVarData const* {
	auto&& opt = hsx::param_data_to_mp_var(param_data);
	if (!opt) {
		assert(false && u8"Casting to local var");
		throw new std::bad_cast{};
	}
	return *opt;
}

auto HspDebugApi::param_data_to_array_var(HspParamData const& param_data) const -> MPVarData const* {
	return param_data_to_single_var(param_data);
}

auto HspDebugApi::param_data_to_mod_var(HspParamData const& param_data) const -> std::optional<MPModVarData const*> {
	return hsx::param_data_to_mp_mod_var(param_data);
}

auto HspDebugApi::param_data_to_data(HspParamData const& param_data) const -> std::optional<HspData> {
	return hsx::param_data_to_data(param_data);
}
