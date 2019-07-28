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
	auto file_ref_name = debug_->fname;

	if (file_ref_name == nullptr || std::strcmp(file_ref_name, u8"???") == 0) {
		return std::nullopt;
	}

	return std::make_optional(file_ref_name);
}

auto HspDebugApi::current_line() const -> std::size_t {
	// 1-indexed。ただしファイルがないときは 0。
	auto line_number = debug_->line;

	return (std::size_t)std::max(0, line_number - 1);
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
	switch (system_var_kind) {
	case HspSystemVarKind::Cnt:
		{
			// FIXME: looplev == 0 のとき？
			return std::make_optional(hsx::data_from_int(&context()->mem_loop[context()->looplev].cnt));
		}

	case HspSystemVarKind::Err:
		{
			static_assert(sizeof(context()->err) == sizeof(HspInt), u8"HSPERROR は int のはず");
			auto ptr = UNSAFE((HspInt*)(&context()->err));
			return std::make_optional(hsx::data_from_int(ptr));
		}

	case HspSystemVarKind::IParam:
		return std::make_optional(hsx::data_from_int(&context()->iparam));

	case HspSystemVarKind::WParam:
		return std::make_optional(hsx::data_from_int(&context()->wparam));

	case HspSystemVarKind::LParam:
		return std::make_optional(hsx::data_from_int(&context()->lparam));

	case HspSystemVarKind::LoopLev:
		return std::make_optional(hsx::data_from_int(&context()->looplev));

	case HspSystemVarKind::SubLev:
		return std::make_optional(hsx::data_from_int(&context()->sublev));

	case HspSystemVarKind::Refstr:
		return std::make_optional(hsx::data_from_str(context()->refstr));

	case HspSystemVarKind::Refdval:
		return std::make_optional(hsx::data_from_double(&context()->refdval));

	case HspSystemVarKind::Stat:
		return std::make_optional(hsx::data_from_int(&context()->stat));

	case HspSystemVarKind::StrSize:
		return std::make_optional(hsx::data_from_int(&context()->strsize));

	case HspSystemVarKind::Thismod: {
		auto mod_var_data_opt = hsx::param_data_to_mp_mod_var(MPTYPE_MODULEVAR, ctx->prmstack);
		if (!mod_var_data_opt) {
			return std::nullopt;
		}

		auto&& data = var_element_to_data((**mod_var_data_opt).pval, (std::size_t)(**mod_var_data_opt).aptr);
		if (data.type() != HspType::Struct) {
			return std::nullopt;
		}

		return std::make_optional(data);
	}
	default:
		assert(false && u8"Invalid HspSystemVarKind");
		throw std::exception{};
	}
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
	assert(flex != nullptr);
	return !flex->ptr || flex->type == FLEXVAL_TYPE_NONE;
}

bool HspDebugApi::flex_is_clone(FlexValue const* flex) const {
	assert(flex != nullptr);
	return flex->type == FLEXVAL_TYPE_CLONE;
}

auto HspDebugApi::flex_to_module_struct(FlexValue const* flex) const -> STRUCTDAT const* {
	return hpiutil::FlexValue_module(flex);
}

auto HspDebugApi::flex_to_module_tag(FlexValue const* flex) const -> STRUCTPRM const* {
	return hpiutil::FlexValue_structTag(flex);
}

auto HspDebugApi::flex_to_member_count(FlexValue const* flex) const -> std::size_t {
	assert(flex != nullptr);

	auto struct_dat = flex_to_module_struct(flex);
	auto param_count = hsx::struct_to_param_count(struct_dat);

	// NOTE: STRUCT_TAG というダミーのパラメータがあるため、メンバ変数の個数は1つ少ない。
	if (param_count == 0) {
		assert(false && u8"STRUCT_TAG を持たないモジュールはモジュール変数のインスタンスを作れないはず");
		return 0;
	}

	return param_count - 1;
}

auto HspDebugApi::flex_to_member_at(FlexValue const* flex, std::size_t member_index) const -> HspParamData {
	auto member_count = flex_to_member_count(flex);
	if (member_index >= member_count) {
		assert(false && u8"Invalid member_index in flex");
		throw new std::exception{};
	}

	// 先頭の STRUCT_TAG を除いて数える。
	auto param_index = member_index + 1;

	auto&& param_stack = flex_to_param_stack(flex);
	auto&& param_data_opt = param_stack_to_data_at(param_stack, param_index);
	if (!param_data_opt) {
		assert(false && u8"Invalid member_index in flex");
		throw new std::exception{};
	}

	return *param_data_opt;
}

auto HspDebugApi::flex_to_param_stack(FlexValue const* flex) const -> HspParamStack {
	auto struct_dat = flex_to_module_struct(flex);
	auto size = hsx::struct_to_param_stack_size(struct_dat);
	auto safety = true;
	return HspParamStack{ struct_dat, flex->ptr, size, safety };
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
	switch (param_type) {
	case MPTYPE_LABEL:
		return u8"label";

	case MPTYPE_DNUM:
		return u8"double";

	case MPTYPE_LOCALSTRING:
		return u8"str";

	case MPTYPE_INUM:
		return u8"int";

	case MPTYPE_SINGLEVAR:
		return u8"var";

	case MPTYPE_ARRAYVAR:
		return u8"array";

	case MPTYPE_LOCALVAR:
		return u8"local";

	case MPTYPE_MODULEVAR:
	case MPTYPE_IMODULEVAR:
	case MPTYPE_TMODULEVAR:
		return u8"modvar";

	default:
		return u8"???";
	}
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
