#include "hpiutil/hpiutil.hpp"
#include "hpiutil/DInfo.hpp"
#include "HspDebugApi.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

static auto param_path_to_param_data(HspObjectPath::Param const& path, HspDebugApi& api) -> std::optional<HspParamData>;

static auto param_path_to_param_type(HspObjectPath::Param const& path, HspDebugApi& api) -> std::optional<HspParamType>;

static auto const GLOBAL_MODULE_ID = std::size_t{ 0 };

static auto const GLOBAL_MODULE_NAME = "@";

static auto var_name_to_scope_resolution(char const* var_name) -> char const* {
	return std::strchr(var_name, '@');
}

// 変数をモジュールごとに分類する。
static auto group_vars_by_module(std::vector<HspString> const& var_names) -> std::vector<HspObjects::Module> {
	auto modules = std::vector<HspObjects::Module>{};

	// モジュール名、変数名、変数IDの組
	auto tuples = std::vector<std::tuple<HspStringView, HspStringView, std::size_t>>{};

	{
		for (auto vi = std::size_t{}; vi < var_names.size(); vi++) {
			auto&& var_name = var_names[vi];

			auto p = var_name_to_scope_resolution(var_name.data());
			auto module_name = HspStringView{ (p ? p : GLOBAL_MODULE_NAME) };

			tuples.emplace_back(module_name, var_name.as_ref(), vi);
		}
	}

	std::sort(std::begin(tuples), std::end(tuples));

	// モジュールと変数の関係を構築する。
	{
		modules.emplace_back(HspStringView{ GLOBAL_MODULE_NAME }.to_owned());

		for (auto&& t : tuples) {
			auto module_name_ref = std::get<0>(t);
			auto var_name_ref = std::get<1>(t);
			auto vi = std::get<2>(t);

			{
				auto module_id = modules.size() - 1;
				if (!(modules[module_id].name() == module_name_ref)) {
					modules.emplace_back(module_name_ref.to_owned());
				}
			}

			{
				auto module_id = modules.size() - 1;
				assert(modules[module_id].name() == module_name_ref);
				modules[module_id].add_var(vi);
			}
		}
	}

	// 事後条件
	assert(modules[GLOBAL_MODULE_ID].name() == HspStringView{ "@" });
	return modules;
}

static auto create_type_datas() -> std::vector<HspObjects::TypeData> {
	auto types = std::vector<HspObjects::TypeData>{};
	types.emplace_back(HspStringView{ "unknown" }.to_owned());
	types.emplace_back(HspStringView{ "label" }.to_owned());
	types.emplace_back(HspStringView{ "str" }.to_owned());
	types.emplace_back(HspStringView{ "double" }.to_owned());
	types.emplace_back(HspStringView{ "int" }.to_owned());
	types.emplace_back(HspStringView{ "struct" }.to_owned());
	types.emplace_back(HspStringView{ "comobj" }.to_owned());
	return types;
}

static auto path_to_pval(HspObjectPath const& path, HspDebugApi& api) -> std::optional<PVal*> {
	switch (path.kind()) {
	case HspObjectKind::StaticVar:
		{
			auto static_var_id = path.as_static_var().static_var_id();
			return std::make_optional(api.static_var_to_pval(static_var_id));
		}

	case HspObjectKind::Element:
		return path_to_pval(path.parent(), api);

	case HspObjectKind::Param:
		{
			auto&& param_data_opt = param_path_to_param_data(path.as_param(), api);
			if (!param_data_opt) {
				return std::nullopt;
			}
			auto&& param_data = *param_data_opt;

			if (api.param_data_to_type(param_data) == MPTYPE_LOCALVAR) {
				auto pval = api.param_data_as_local_var(param_data);
				return std::make_optional(pval);
			}
			return std::nullopt;
		}
	default:
		return std::nullopt;
	}
}

static auto path_to_data(HspObjectPath const& path, HspDebugApi& api) -> std::optional<HspData> {
	// FIXME: 静的変数も値を提供できる

	switch (path.kind()) {
	case HspObjectKind::Element:
		{
			auto&& pval_opt = path_to_pval(path.parent(), api);
			if (!pval_opt) {
				assert(false && u8"配列要素の親は変数であるはず");
				return std::nullopt;
			}

			auto aptr = api.var_element_to_aptr(*pval_opt, path.as_element().indexes());
			return std::make_optional(api.var_element_to_data(*pval_opt, aptr));
		}
	case HspObjectKind::Param:
	{
		auto&& param_data_opt = param_path_to_param_data(path.as_param(), api);
		if (!param_data_opt) {
			return std::nullopt;
		}

		return api.param_data_to_data(*param_data_opt);
	}
	case HspObjectKind::SystemVar:
	{
		return api.system_var_to_data(path.as_system_var().system_var_kind());
	}
	default:
		return std::nullopt;
	}
}

static auto var_path_to_child_count(HspObjectPath const& path, HspDebugApi& api) -> std::size_t {
	auto&& pval_opt = path_to_pval(path, api);
	if (!pval_opt) {
		return 0;
	}

	auto pval = *pval_opt;
	return api.var_to_element_count(pval);
}

static auto var_path_to_child_at(HspObjectPath const& path, std::size_t child_index, HspDebugApi& api) -> std::shared_ptr<HspObjectPath const> {
	auto pval_opt = path_to_pval(path, api);
	if (!pval_opt || child_index >= var_path_to_child_count(path, api)) {
		assert(false && u8"Invalid var path child index");
		throw new std::out_of_range{ u8"child_index" };
	}

	auto pval = *pval_opt;
	auto aptr = (APTR)child_index;
	auto&& indexes = api.var_element_to_indexes(pval, aptr);
	return path.new_element(indexes);
}

static auto label_path_to_value(HspObjectPath::Label const& path, HspDebugApi& api) -> std::optional<HspLabel> {
	auto&& data_opt = path_to_data(path.parent(), api);
	if (!data_opt) {
		assert(false && u8"label の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data_opt->type() != HspType::Label) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_label(*data_opt));
}

static auto str_path_to_value(HspObjectPath::Str const& path, HspDebugApi& api) -> std::optional<HspStr> {
	auto&& data = path_to_data(path.parent(), api);
	if (!data) {
		assert(false && u8"str の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Str) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_str(*data));
}

static auto double_path_to_value(HspObjectPath::Double const& path, HspDebugApi& api) -> std::optional<HspDouble> {
	auto&& data = path_to_data(path.parent(), api);
	if (!data) {
		assert(false && u8"double の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Double) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_double(*data));
}

static auto int_path_to_value(HspObjectPath::Int const& path, HspDebugApi& api) -> std::optional<HspInt> {
	auto&& data = path_to_data(path.parent(), api);
	if (!data) {
		assert(false && u8"int の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Int) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_int(*data));
}

static auto flex_path_to_value(HspObjectPath::Flex const& path, HspDebugApi& api) -> std::optional<FlexValue*> {
	auto&& data = path_to_data(path.parent(), api);
	if (!data) {
		assert(false && u8"flex の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Struct) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_flex(*data));
}

static auto path_to_param_stack(HspObjectPath const& path, HspDebugApi& api) -> std::optional<HspParamStack> {
	// FIXME: スタックフレームからも取れる。

	switch (path.kind()) {
	case HspObjectKind::Flex:
		{
			auto&& flex_opt = flex_path_to_value(path.as_flex(), api);
			if (!flex_opt) {
				return std::nullopt;
			}
			return std::make_optional<HspParamStack>(api.flex_to_param_stack(*flex_opt));
		}
	default:
		return std::nullopt;
	}
}

static auto param_path_to_param_data(HspObjectPath::Param const& path, HspDebugApi& api) -> std::optional<HspParamData> {
	auto&& parent = path.parent();

	auto&& param_stack = path_to_param_stack(parent, api);
	if (!param_stack) {
		assert(false && u8"param の親要素から param_stack が取れるはず");
		return std::nullopt;
	}

	return api.param_stack_to_data_at(*param_stack, path.param_index());
}

static auto param_path_to_param_type(HspObjectPath::Param const& path, HspDebugApi& api) -> std::optional<HspParamType> {
	auto&& param_data_opt = param_path_to_param_data(path, api);
	if (!param_data_opt) {
		return std::nullopt;
	}

	return std::make_optional(api.param_data_to_type(*param_data_opt));
}

// -----------------------------------------------
// HspObjects
// -----------------------------------------------

HspObjects::HspObjects(HspDebugApi& api, HspLogger& logger, HspScripts& scripts, HspStaticVars& static_vars, hpiutil::DInfo const& debug_segment)
	: api_(api)
	, logger_(logger)
	, scripts_(scripts)
	, static_vars_(static_vars)
	, debug_segment_(debug_segment)
	, root_path_(std::make_shared<HspObjectPath::Root>())
	, modules_(group_vars_by_module(static_vars.get_all_names()))
	, types_(create_type_datas())
{
}

auto HspObjects::root_path() const->HspObjectPath::Root const& {
	return root_path_->as_root();
}

auto HspObjects::type_to_name(HspType type) const->HspStringView {
	auto type_id = (std::size_t)type;
	if (!(1 <= type_id && type_id < types_.size())) {
		return types_[0].name();
	}
	return types_[type_id].name();
}

auto HspObjects::module_global_id() const->std::size_t {
	return GLOBAL_MODULE_ID;
}

auto HspObjects::module_count() const->std::size_t {
	return modules_.size();
}

auto HspObjects::module_to_name(std::size_t module_id) const -> HspStringView {
	return modules_.at(module_id).name();
}

auto HspObjects::module_to_var_count(std::size_t module_id) const->std::size_t {
	return modules_.at(module_id).var_ids().size();
}

auto HspObjects::module_to_var_at(std::size_t module_id, std::size_t index) const->std::size_t {
	return modules_.at(module_id).var_ids().at(index);
}

auto HspObjects::static_var_path_to_name(HspObjectPath::StaticVar const& path)->std::string {
	return *api_.static_var_find_name(path.static_var_id());
}

bool HspObjects::static_var_path_is_array(HspObjectPath::StaticVar const& path) {
	return api_.var_is_array(api_.static_var_to_pval(path.static_var_id()));
}

auto HspObjects::static_var_path_to_pval(HspObjectPath::StaticVar const& path)->PVal* {
	return api_.static_var_to_pval(path.static_var_id());
}

auto HspObjects::static_var_path_to_type(HspObjectPath::StaticVar const& path)->HspType {
	return api_.var_to_type(api_.static_var_to_pval(path.static_var_id()));
}

auto HspObjects::static_var_path_to_child_count(HspObjectPath::StaticVar const& path) const->std::size_t {
	return var_path_to_child_count(path, api_);
}

auto HspObjects::static_var_path_to_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index) const->std::shared_ptr<HspObjectPath const> {
	return var_path_to_child_at(path, child_index, api_);
}

auto HspObjects::static_var_path_to_metadata(HspObjectPath::StaticVar const& path) -> HspVarMetadata {
	auto pval = static_var_path_to_pval(path);
	auto block_memory = api_.var_to_block_memory(pval);

	auto metadata = HspVarMetadata{};
	metadata.lengths_ = api_.var_to_lengths(pval);
	metadata.element_size_ = api_.var_to_element_count(pval);
	metadata.data_size_ = pval->size;
	metadata.block_size_ = block_memory.size();
	metadata.data_ptr_ = pval->pt;
	metadata.master_ptr_ = pval->master;
	metadata.block_ptr_ = block_memory.data();
	return metadata;
}

auto HspObjects::element_path_to_child_count(HspObjectPath::Element const& path) const -> std::size_t {
	auto&& pval_opt = path_to_pval(path, api_);
	if (!pval_opt) {
		return 0;
	}

	return 1;
}

auto HspObjects::element_path_to_child_at(HspObjectPath::Element const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < element_path_to_child_count(path));

	auto&& pval_opt = path_to_pval(path, api_);
	if (!pval_opt) {
		assert(false && u8"Invalid element path child index");
		throw new std::out_of_range{ u8"child_index" };
	}

	auto type = api_.var_to_type(*pval_opt);
	switch (type) {
	case HspType::Label:
		return path.new_label();
	case HspType::Str:
		return path.new_str();
	case HspType::Double:
		return path.new_double();
	case HspType::Int:
		return path.new_int();
	case HspType::Struct:
		return path.new_flex();
	default:
		return path.new_unknown();
	}
}

auto HspObjects::param_path_to_child_count(HspObjectPath::Param const& path) const -> std::size_t {
	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
		return var_path_to_child_count(path, api_);
	case MPTYPE_LOCALSTRING:
	case MPTYPE_INUM:
		return 1;
	default:
		// FIXME: 他の種類の実装
		return 0;
	}
}

auto HspObjects::param_path_to_child_at(HspObjectPath::Param const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < param_path_to_child_count(path));

	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
		return var_path_to_child_at(path, child_index, api_);
	case MPTYPE_LOCALSTRING:
		return path.new_str();
	case MPTYPE_INUM:
		return path.new_int();
	default:
		assert(false && u8"Invalid param path child index");
		throw new std::out_of_range{ u8"child_index" };
	}
}

auto HspObjects::param_path_to_name(HspObjectPath::Param const& path) const -> std::string {
	auto&& param_data = param_path_to_param_data(path, api_);
	return api_.param_to_name(param_data->param(), debug_segment_);
}

bool HspObjects::label_path_is_null(HspObjectPath::Label const& path) const {
	auto&& label_opt = label_path_to_value(path, api_);
	if (!label_opt) {
		return true;
	}

	return *label_opt == nullptr;
}

auto HspObjects::label_path_to_static_label_name(HspObjectPath::Label const& path) const -> std::optional<std::string> {
	auto&& static_label_id_opt = label_path_to_static_label_id(path);
	if (!static_label_id_opt) {
		return std::nullopt;
	}

	auto&& name = debug_segment_.tryFindLabelName((int)*static_label_id_opt);
	if (!name) {
		return std::nullopt;
	}

	// FIXME: 効率化 (文字列の参照かビューを返す)
	return std::make_optional(std::string{ name });
}

auto HspObjects::label_path_to_static_label_id(HspObjectPath::Label const& path) const -> std::optional<std::size_t> {
	auto&& label_opt = label_path_to_value(path, api_);
	if (!label_opt) {
		return std::nullopt;
	}

	// FIXME: 効率化 (事前にハッシュテーブルをつくる)
	for (auto id = std::size_t{}; id < api_.static_label_count(); id++) {
		auto&& opt = api_.static_label_to_label(id);
		if (!opt) {
			assert(false && u8"id must be valid");
			continue;
		}

		if (*opt == *label_opt) {
			return std::make_optional(id);
		}
	}

	return std::nullopt;
}

auto HspObjects::str_path_to_value(HspObjectPath::Str const& path) const -> HspStr {
	static char empty[64]{};
	return (::str_path_to_value(path, api_)).value_or(empty);
}

auto HspObjects::double_path_to_value(HspObjectPath::Double const& path) const->HspDouble {
	return (::double_path_to_value(path, api_)).value_or(HspDouble{});
}

auto HspObjects::int_path_to_value(HspObjectPath::Int const& path) const -> HspInt {
	return (::int_path_to_value(path, api_)).value_or(HspInt{});
}

auto HspObjects::flex_path_to_child_count(HspObjectPath::Flex const& path)->std::size_t {
	auto&& flex_opt = flex_path_to_value(path, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		return 0;
	}

	return api_.flex_to_member_count(*flex_opt);
}

auto HspObjects::flex_path_to_child_at(HspObjectPath::Flex const& path, std::size_t index)->std::shared_ptr<HspObjectPath const> {
	auto&& flex_opt = flex_path_to_value(path, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		assert(false && u8"Invalid flex path child index");
		throw new std::out_of_range{ u8"child_index" };
	}

	auto&& param_data = api_.flex_to_member_at(*flex_opt, index);
	auto param_type = api_.param_data_to_type(param_data);
	auto param_index = param_data.param_index();
	return path.new_param(param_type, param_index);
}

bool HspObjects::flex_path_is_nullmod(HspObjectPath::Flex const& path) {
	auto&& flex_opt = flex_path_to_value(path, api_);
	if (!flex_opt) {
		return true;
	}

	return api_.flex_is_nullmod(*flex_opt);
}

auto HspObjects::flex_path_to_module_name(HspObjectPath::Flex const& path) -> char const* {
	auto&& flex_opt = flex_path_to_value(path, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		return "null";
	}

	auto struct_dat = api_.flex_to_module_struct(*flex_opt);
	return api_.struct_to_name(struct_dat);
}

auto HspObjects::system_var_path_to_child_count(HspObjectPath::SystemVar const& path) const -> std::size_t {
	switch (path.system_var_kind()) {
	case HspSystemVarKind::Refdval:
	case HspSystemVarKind::Refstr:
	case HspSystemVarKind::Stat:
		return 1;
	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_child_at(HspObjectPath::SystemVar const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < system_var_path_to_child_count(path));

	switch (path.system_var_kind()) {
	case HspSystemVarKind::Refdval:
		return path.new_double();
	case HspSystemVarKind::Refstr:
		return path.new_str();
	case HspSystemVarKind::Stat:
		return path.new_int();
	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_name(HspObjectPath::SystemVar const& path) const -> std::string {
	switch (path.system_var_kind()) {
	case HspSystemVarKind::Refdval:
		return std::string{ u8"refdval" };
	case HspSystemVarKind::Refstr:
		return std::string{ u8"refstr" };
	case HspSystemVarKind::Stat:
		return std::string{ u8"stat" };
	default:
		assert(false && u8"Invalid HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::log_to_content() const -> std::string const& {
	return logger_.content();
}

void HspObjects::log_do_append(char const* text) {
	logger_.append(text);
}

void HspObjects::log_do_clear() {
	logger_.clear();
}

auto HspObjects::script_to_content() const -> std::string const& {
	auto file_ref_name = api_.current_file_ref_name().value_or("");
	return scripts_.content(file_ref_name);
}

auto HspObjects::script_to_current_line() const -> std::size_t {
	return api_.current_line();
}

// -----------------------------------------------
// HspObjects::Module
// -----------------------------------------------

HspObjects::Module::Module(HspString&& name)
	: name_(std::move(name))
	, var_ids_()
{
}

void HspObjects::Module::add_var(std::size_t static_var_id) {
	var_ids_.emplace_back(static_var_id);
}

// -----------------------------------------------
// HspObjects::TypeData
// -----------------------------------------------

HspObjects::TypeData::TypeData(HspString&& name)
	: name_(std::move(name))
{
}
