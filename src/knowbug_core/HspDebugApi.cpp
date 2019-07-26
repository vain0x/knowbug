#include "pch.h"
#include "../hpiutil/hpiutil.hpp"
#include "../hpiutil/dinfo.hpp"
#include "HspDebugApi.h"

#undef max

#define UNSAFE(E) (E)

namespace hsx = hsp_sdk_ext;

static auto param_data_to_mod_var_data(HspParamType type, void const* data) -> std::optional<MPModVarData*> {
	if (type != MPTYPE_MODULEVAR && type != MPTYPE_IMODULEVAR && type != MPTYPE_TMODULEVAR) {
		return std::nullopt;
	}

	auto mod_var_data = UNSAFE((MPModVarData*)data);
	if (!mod_var_data || mod_var_data->magic != MODVAR_MAGICCODE || !mod_var_data->pval || mod_var_data->aptr < 0) {
		return std::nullopt;
	}

	return std::make_optional(mod_var_data);
}

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

auto HspDebugApi::static_vars() -> PVal* {
	return context()->mem_var;
}

auto HspDebugApi::static_var_count() -> std::size_t {
	return context()->hsphed->max_val;
}

auto HspDebugApi::static_var_find_by_name(char const* var_name) -> std::optional<std::size_t> {
	assert(var_name != nullptr);

	auto index = exinfo()->HspFunc_seekvar(var_name);

	if (index < 0 || (std::size_t)index >= static_var_count()) {
		return std::nullopt;
	}

	return std::make_optional((std::size_t)index);
}

auto HspDebugApi::static_var_find_name(std::size_t static_var_id) -> std::optional<std::string> {
	assert(static_var_id < static_var_count());

	auto var_name = exinfo()->HspFunc_varname((int)static_var_id);
	if (!var_name) {
		return std::nullopt;
	}

	return std::make_optional<std::string>(var_name);
}

auto HspDebugApi::static_var_to_pval(std::size_t static_var_id) -> PVal* {
	if (static_var_id >= static_var_count()) {
		assert(false && u8"Unknown static_var_id");
		throw new std::exception{};
	}

	return static_vars() + static_var_id;
}

auto HspDebugApi::var_to_type(PVal* pval) -> HspType {
	return hsx::pval_to_type(pval);
}

auto HspDebugApi::var_to_data(PVal* pval) -> HspData {
	return hsx::pval_to_data(pval, context()).value_or(HspData{});
}

auto HspDebugApi::var_to_lengths(PVal* pval) const -> HspDimIndex {
	return hsx::pval_to_lengths(pval);
}

auto HspDebugApi::var_to_mode(PVal* pval) const -> HspVarMode {
	return hsx::pval_to_varmode(pval);
}

bool HspDebugApi::var_is_array(PVal* pval) const {
	return hsx::pval_is_standard_array(pval, context());
}

auto HspDebugApi::var_to_element_count(PVal* pval) -> std::size_t {
	return hsx::pval_to_element_count(pval);
}

auto HspDebugApi::var_element_to_indexes(PVal* pval, std::size_t aptr) -> std::optional<HspDimIndex> {
	return hsx::element_to_indexes(pval, aptr);
}

auto HspDebugApi::var_element_to_aptr(PVal* pval, HspDimIndex const& indexes) -> std::optional<std::size_t> {
	return hsx::element_to_aptr(pval, indexes);
}

auto HspDebugApi::var_element_to_data(PVal* pval, std::size_t aptr) -> HspData {
	return hsx::element_to_data(pval, aptr, context()).value_or(HspData{});
}

auto HspDebugApi::var_to_block_memory(PVal* pval) -> MemoryView {
	return hsx::pval_to_memory_block(pval, context());
}

auto HspDebugApi::var_element_to_block_memory(PVal* pval, std::size_t aptr) -> MemoryView {
	return hsx::element_to_memory_block(pval, aptr, context());
}

auto HspDebugApi::mp_var_data_to_pval(MPVarData* var_data) -> PVal* {
	assert(var_data != nullptr);
	assert(var_data->pval != nullptr);
	return var_data->pval;
}

auto HspDebugApi::mp_var_data_to_aptr(MPVarData* var_data) -> std::size_t {
	assert(var_data != nullptr);
	assert(var_data->pval != nullptr);
	assert(var_data->aptr >= 0);
	return (std::size_t)var_data->aptr;
}

auto HspDebugApi::mp_mod_var_data_to_pval(MPModVarData* mod_var_data) -> PVal* {
	assert(mod_var_data != nullptr);
	assert(mod_var_data->magic == MODVAR_MAGICCODE);
	assert(mod_var_data->pval != nullptr);
	return mod_var_data->pval;
}

auto HspDebugApi::mp_mod_var_data_to_aptr(MPModVarData* mod_var_data) -> std::size_t {
	assert(mod_var_data != nullptr);
	assert(mod_var_data->magic == MODVAR_MAGICCODE);
	assert(mod_var_data->aptr >= 0);
	return (std::size_t)mod_var_data->aptr;
}

auto HspDebugApi::system_var_to_data(HspSystemVarKind system_var_kind) -> std::optional<HspData> {
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
		auto mod_var_data_opt = param_data_to_mod_var_data(MPTYPE_MODULEVAR, ctx->prmstack);
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
	if (data.type() != HspType::Label) {
		assert(false && u8"Invalid cast to label");
		throw new std::bad_cast{};
	}

	return UNSAFE(*(HspLabel const*)data.ptr());
}

auto HspDebugApi::data_to_str(HspData const& data) const -> HspStr {
	if (data.type() != HspType::Str) {
		assert(false && u8"Invalid cast to string");
		throw new std::bad_cast{};
	}

	return UNSAFE((HspStr)data.ptr());
}

auto HspDebugApi::data_to_double(HspData const& data) const -> HspDouble {
	if (data.type() != HspType::Double) {
		assert(false && u8"Invalid cast to double");
		throw new std::bad_cast{};
	}

	return UNSAFE(*(HspDouble const*)data.ptr());
}

auto HspDebugApi::data_to_int(HspData const& data) const -> HspInt {
	if (data.type() != HspType::Int) {
		assert(false && u8"Invalid cast to int");
		throw new std::bad_cast{};
	}

	return UNSAFE(*(HspInt const*)data.ptr());
}

auto HspDebugApi::data_to_flex(HspData const& data) const -> FlexValue* {
	if (data.type() != HspType::Struct) {
		assert(false && u8"Invalid cast to struct");
		throw new std::bad_cast{};
	}

	return UNSAFE((FlexValue*)data.ptr());
}

auto HspDebugApi::static_labels() -> HspCodeOffset const* {
	return context()->mem_ot;
}

auto HspDebugApi::static_label_count() -> std::size_t {
	if (context()->hsphed->max_ot < 0) {
		assert(false && u8"max_ot must be positive.");
		throw std::exception{};
	}

	return (std::size_t)context()->hsphed->max_ot;
}

auto HspDebugApi::static_label_to_label(std::size_t static_label_id) -> std::optional<HspLabel> {
	if (static_label_id >= static_label_count()) {
		return std::nullopt;
	}

	return std::make_optional(context()->mem_mcs + context()->mem_ot[static_label_id]);
}

bool HspDebugApi::flex_is_nullmod(FlexValue* flex) const {
	assert(flex != nullptr);
	return !flex->ptr || flex->type == FLEXVAL_TYPE_NONE;
}

bool HspDebugApi::flex_is_clone(FlexValue* flex) const {
	assert(flex != nullptr);
	return flex->type == FLEXVAL_TYPE_CLONE;
}

auto HspDebugApi::flex_to_module_struct(FlexValue* flex) const -> STRUCTDAT const* {
	return hpiutil::FlexValue_module(flex);
}

auto HspDebugApi::flex_to_module_tag(FlexValue* flex) const -> STRUCTPRM const* {
	return hpiutil::FlexValue_structTag(flex);
}

auto HspDebugApi::flex_to_member_count(FlexValue* flex) const -> std::size_t {
	assert(flex != nullptr);

	auto struct_dat = flex_to_module_struct(flex);
	auto param_count = struct_to_param_count(struct_dat);

	// NOTE: STRUCT_TAG というダミーのパラメータがあるため、メンバ変数の個数は1つ少ない。
	if (param_count == 0) {
		assert(false && u8"STRUCT_TAG を持たないモジュールはモジュール変数のインスタンスを作れないはず");
		return 0;
	}

	return param_count - 1;
}

auto HspDebugApi::flex_to_member_at(FlexValue* flex, std::size_t member_index) const -> HspParamData {
	auto member_count = flex_to_member_count(flex);
	if (member_index >= member_count) {
		assert(false && u8"Invalid member_index in flex");
		throw new std::exception{};
	}

	// 先頭の STRUCT_TAG を除いて数える。
	auto param_index = member_index + 1;

	auto&& param_stack = flex_to_param_stack(flex);
	return param_stack_to_data_at(param_stack, param_index);
}

auto HspDebugApi::flex_to_param_stack(FlexValue* flex) const -> HspParamStack {
	auto struct_dat = flex_to_module_struct(flex);
	auto size = struct_to_param_stack_size(struct_dat);
	auto safety = true;
	return HspParamStack{ struct_dat, flex->ptr, size, safety };
}

auto HspDebugApi::structs() const -> STRUCTDAT const* {
	return hpiutil::finfo().begin();
}

auto HspDebugApi::struct_count() const -> std::size_t {
	return hpiutil::finfo().size();
}

auto HspDebugApi::struct_to_name(STRUCTDAT const* struct_dat) const -> char const* {
	return hpiutil::STRUCTDAT_name(struct_dat);
}

auto HspDebugApi::struct_to_param_count(STRUCTDAT const* struct_dat) const -> std::size_t {
	return hpiutil::STRUCTDAT_params(struct_dat).size();
}

auto HspDebugApi::struct_to_param_at(STRUCTDAT const* struct_dat, std::size_t param_index) const -> STRUCTPRM const* {
	return hpiutil::STRUCTDAT_params(struct_dat).begin() + param_index;
}

auto HspDebugApi::struct_to_param_stack_size(STRUCTDAT const* struct_dat) const -> std::size_t {
	assert(struct_dat != nullptr);
	return struct_dat->size;
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
	auto struct_dat = hpiutil::STRUCTPRM_stdat(param);
	return hpiutil::nameFromStPrm(param, (int)param_index, debug_segment);
}

auto HspDebugApi::param_stack_to_data_count(HspParamStack const& param_stack) const -> std::size_t {
	return hpiutil::STRUCTDAT_params(param_stack.struct_dat()).size();
}

auto HspDebugApi::param_stack_to_data_at(HspParamStack const& param_stack, std::size_t param_index) const -> HspParamData {
	if (param_index >= param_stack_to_data_count(param_stack)) {
		assert(false && u8"Invalid param_index");
		throw new std::exception{};
	}

	auto param = hpiutil::STRUCTDAT_params(param_stack.struct_dat()).begin() + param_index;
	auto ptr = (void*)((char const*)param_stack.ptr() + param->offset);
	return HspParamData{ param, param_index, ptr, param_stack.safety() };
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
	return param_data.param()->mptype;
}

auto HspDebugApi::param_data_as_local_var(HspParamData const& param_data) const -> PVal* {
	if (param_data_to_type(param_data) != MPTYPE_LOCALVAR) {
		assert(false && u8"Casting to local var");
		throw new std::bad_cast{};
	}
	return UNSAFE((PVal*)param_data.ptr());
}

auto HspDebugApi::param_data_to_single_var(HspParamData const& param_data) const -> MPVarData* {
	auto type = param_data_to_type(param_data);
	if (type != MPTYPE_SINGLEVAR && type != MPTYPE_ARRAYVAR) {
		assert(false && u8"Casting to local var");
		throw new std::bad_cast{};
	}
	return UNSAFE((MPVarData*)param_data.ptr());
}

auto HspDebugApi::param_data_to_array_var(HspParamData const& param_data) const -> MPVarData* {
	return param_data_to_single_var(param_data);
}

auto HspDebugApi::param_data_to_mod_var(HspParamData const& param_data) const -> std::optional<MPModVarData*> {
	auto type = param_data_to_type(param_data);
	auto&& mod_var_data_opt = param_data_to_mod_var_data(type, param_data.ptr());
	if (!mod_var_data_opt) {
		return std::nullopt;
	}
	return *mod_var_data_opt;
}

auto HspDebugApi::param_data_to_data(HspParamData const& param_data) const -> std::optional<HspData> {
	if (!param_data.safety()) {
		return std::nullopt;
	}

	switch (param_data_to_type(param_data)) {
	case MPTYPE_LABEL:
		{
			auto ptr = UNSAFE((HspLabel*)param_data.ptr());
			return std::make_optional(hsx::data_from_label(ptr));
		}
	case MPTYPE_LOCALSTRING:
		{
			auto str = UNSAFE(*(char**)param_data.ptr());
			if (!str) {
				assert(false && u8"str param must not be null");
				return std::nullopt;
			}
			return std::make_optional(hsx::data_from_str(str));
		}
	case MPTYPE_DNUM:
		{
			auto ptr = UNSAFE((HspDouble*)param_data.ptr());
			return std::make_optional(hsx::data_from_double(ptr));
		}
	case MPTYPE_INUM:
		{
			auto ptr = UNSAFE((HspInt*)param_data.ptr());
			return std::make_optional(hsx::data_from_int(ptr));
		}
	default:
		return std::nullopt;
	}
}
