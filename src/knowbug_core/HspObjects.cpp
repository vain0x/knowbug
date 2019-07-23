#include "pch.h"
#include <sstream>
#include "../hpiutil/hpiutil.hpp"
#include "../hpiutil/DInfo.hpp"
#include "hsp_wrap_call.h"
#include "DebugInfo.h"
#include "hsp_objects_module_tree.h"
#include "HspDebugApi.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

// 再帰深度の初期値
static auto const MIN_DEPTH = std::size_t{};

// 再帰深度の最大値 (スタックオーバーフローを防ぐため)
static auto const MAX_DEPTH = std::size_t{ 32 };

static auto param_path_to_param_data(HspObjectPath::Param const& path, std::size_t depth, HspDebugApi& api) -> std::optional<HspParamData>;

static auto param_path_to_param_type(HspObjectPath::Param const& path, HspDebugApi& api) -> std::optional<HspParamType>;

static auto const GLOBAL_MODULE_ID = std::size_t{ 0 };

// 変数をモジュールごとに分類する。
static auto group_vars_by_module(std::vector<Utf8String> const& var_names) -> std::vector<HspObjects::Module> {
	class ModuleTreeBuilder
		: public ModuleTreeListener
	{
		std::vector<HspObjects::Module>& modules_;

	public:
		ModuleTreeBuilder(std::vector<HspObjects::Module>& modules)
			: modules_(modules)
		{
		}

		void begin_module(Utf8StringView const& module_name) override {
			modules_.emplace_back(HspObjects::Module{ to_owned(module_name) });
		}

		void end_module() override {
		}

		void add_var(std::size_t var_id, Utf8StringView const& var_name) override {
			modules_.back().add_var(var_id);
		}
	};

	auto modules = std::vector<HspObjects::Module>{};
	auto builder = ModuleTreeBuilder{ modules };

	traverse_module_tree(var_names, builder);

	assert(!modules.empty());
	return modules;
}

static auto create_type_datas() -> std::vector<HspObjects::TypeData> {
	auto types = std::vector<HspObjects::TypeData>{};
	types.emplace_back(ascii_to_utf8(u8"unknown"));
	types.emplace_back(ascii_to_utf8(u8"label"));
	types.emplace_back(ascii_to_utf8(u8"str"));
	types.emplace_back(ascii_to_utf8(u8"double"));
	types.emplace_back(ascii_to_utf8(u8"int"));
	types.emplace_back(ascii_to_utf8(u8"struct"));
	types.emplace_back(ascii_to_utf8(u8"comobj"));
	return types;
}

static auto create_general_content(DebugInfo const& debug_info) -> Utf8String {
	auto kvs = debug_info.fetchGeneralInfo();

	auto buffer = Utf8String{};
	for (auto&& kv : kvs) {
		buffer += to_utf8(as_hsp(std::move(kv.first)));
		buffer += as_utf8(u8" = ");
		buffer += to_utf8(as_hsp(std::move(kv.second)));
		buffer += as_utf8(u8"\r\n");
	}
	return buffer;
}

static auto path_to_pval(HspObjectPath const& path, std::size_t depth, HspDebugApi& api) -> std::optional<PVal*> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar:
		{
			auto static_var_id = path.as_static_var().static_var_id();
			return std::make_optional(api.static_var_to_pval(static_var_id));
		}

	case HspObjectKind::Element:
		return path_to_pval(path.parent(), depth, api);

	case HspObjectKind::Param:
		{
			auto&& param_data_opt = param_path_to_param_data(path.as_param(), depth, api);
			if (!param_data_opt) {
				return std::nullopt;
			}
			auto&& param_data = *param_data_opt;

			switch (api.param_data_to_type(param_data)) {
			case MPTYPE_LOCALVAR:
				{
					auto pval = api.param_data_as_local_var(param_data);
					return std::make_optional(pval);
				}
			case MPTYPE_SINGLEVAR:
			case MPTYPE_ARRAYVAR:
				{
					auto mp_var_data = api.param_data_to_single_var(param_data);
					auto pval = api.mp_var_data_to_pval(mp_var_data);
					return std::make_optional(pval);
				}
			default:
				return std::nullopt;
			}
		}
	default:
		return std::nullopt;
	}
}

static auto path_to_data(HspObjectPath const& path, std::size_t depth, HspDebugApi& api) -> std::optional<HspData> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	// FIXME: 静的変数も値を提供できる

	switch (path.kind()) {
	case HspObjectKind::Element:
		{
			auto&& pval_opt = path_to_pval(path.parent(), depth, api);
			if (!pval_opt) {
				return std::nullopt;
			}

			auto aptr = api.var_element_to_aptr(*pval_opt, path.as_element().indexes());
			return std::make_optional(api.var_element_to_data(*pval_opt, aptr));
		}
	case HspObjectKind::Param:
	{
		auto&& param_data_opt = param_path_to_param_data(path.as_param(), depth, api);
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
		assert(false && u8"data を取得できるべき");
		return std::nullopt;
	}
}

static auto var_path_to_child_count(HspObjectPath const& path, HspDebugApi& api) -> std::size_t {
	auto&& pval_opt = path_to_pval(path, MIN_DEPTH, api);
	if (!pval_opt) {
		return 0;
	}

	// FIXME: 要素数が多すぎると動作が遅くなりすぎるので適度に打ち切るかグループ化する
	auto pval = *pval_opt;
	return api.var_to_element_count(pval);
}

static auto var_path_to_child_at(HspObjectPath const& path, std::size_t child_index, HspDebugApi& api) -> std::shared_ptr<HspObjectPath const> {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, api);
	if (!pval_opt || child_index >= var_path_to_child_count(path, api)) {
		assert(false && u8"Invalid var path child index");
		throw new std::out_of_range{ u8"child_index" };
	}

	auto pval = *pval_opt;
	auto aptr = (APTR)child_index;
	auto&& indexes = api.var_element_to_indexes(pval, aptr);
	return path.new_element(indexes);
}

static auto var_path_to_metadata(HspObjectPath const& path, HspDebugApi& api) -> std::optional<HspVarMetadata> {
	auto&& pval_opt = path_to_pval(path, MIN_DEPTH, api);
	if (!pval_opt) {
		return std::nullopt;
	}
	auto&& pval = *pval_opt;

	auto block_memory = api.var_to_block_memory(pval);

	auto metadata = HspVarMetadata{};
	metadata.type_ = api.var_to_type(pval);
	metadata.mode_ = api.var_to_mode(pval);
	metadata.lengths_ = api.var_to_lengths(pval);
	metadata.element_size_ = api.var_to_element_count(pval);
	metadata.data_size_ = pval->size;
	metadata.block_size_ = block_memory.size();
	metadata.data_ptr_ = pval->pt;
	metadata.master_ptr_ = pval->master;
	metadata.block_ptr_ = block_memory.data();
	return metadata;
}

static auto label_path_to_value(HspObjectPath::Label const& path, HspDebugApi& api) -> std::optional<HspLabel> {
	auto&& data_opt = path_to_data(path.parent(), MIN_DEPTH, api);
	if (!data_opt) {
		assert(false && u8"label の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data_opt->type() != HspType::Label) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_label(*data_opt));
}

static auto str_path_to_value(HspObjectPath::Str const& path, HspDebugApi& api) -> std::optional<Utf8String> {
	auto&& data = path_to_data(path.parent(), MIN_DEPTH, api);
	if (!data) {
		assert(false && u8"str の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Str) {
		return std::nullopt;
	}

	auto&& str = api.data_to_str(*data);
	return std::make_optional(to_utf8(as_hsp(str)));
}

static auto double_path_to_value(HspObjectPath::Double const& path, HspDebugApi& api) -> std::optional<HspDouble> {
	auto&& data = path_to_data(path.parent(), MIN_DEPTH, api);
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
	auto&& data = path_to_data(path.parent(), MIN_DEPTH, api);
	if (!data) {
		assert(false && u8"int の親は data を生成できるはず");
		return std::nullopt;
	}

	if (data->type() != HspType::Int) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_int(*data));
}

static auto flex_path_to_value(HspObjectPath::Flex const& path, std::size_t depth, HspDebugApi& api) -> std::optional<FlexValue*> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	auto&& data = path_to_data(path.parent(), depth, api);
	if (!data) {
		return std::nullopt;
	}

	if (data->type() != HspType::Struct) {
		return std::nullopt;
	}

	return std::make_optional(api.data_to_flex(*data));
}

static auto path_to_param_stack(HspObjectPath const& path, std::size_t depth, HspDebugApi& api) -> std::optional<HspParamStack> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::Flex:
		{
			auto&& flex_opt = flex_path_to_value(path.as_flex(), depth, api);
			if (!flex_opt) {
				return std::nullopt;
			}
			return std::make_optional<HspParamStack>(api.flex_to_param_stack(*flex_opt));
		}
	case HspObjectKind::CallFrame:
		return wc_call_frame_to_param_stack(path.as_call_frame().call_frame_id());

	default:
		assert(false && u8"param_stack が取れるべき");
		return std::nullopt;
	}
}

static auto param_path_to_param_data(HspObjectPath::Param const& path, std::size_t depth, HspDebugApi& api) -> std::optional<HspParamData> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	auto&& parent = path.parent();

	auto&& param_stack = path_to_param_stack(parent, depth, api);
	if (!param_stack) {
		return std::nullopt;
	}

	return api.param_stack_to_data_at(*param_stack, path.param_index());
}

static auto param_path_to_param_type(HspObjectPath::Param const& path, std::size_t depth, HspDebugApi& api) -> std::optional<HspParamType> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	auto&& param_data_opt = param_path_to_param_data(path, depth, api);
	if (!param_data_opt) {
		return std::nullopt;
	}

	return std::make_optional(api.param_data_to_type(*param_data_opt));
}

static auto param_stack_to_memory_view(HspParamStack const& param_stack) -> MemoryView {
	return MemoryView{ param_stack.ptr(), param_stack.size() };
}

static auto path_to_memory_view(HspObjectPath const& path, std::size_t depth, HspDebugApi& api) -> std::optional<MemoryView> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar:
	case HspObjectKind::Param:
	{
		auto&& pval_opt = path_to_pval(path, depth, api);
		if (!pval_opt) {
			return std::nullopt;
		}
		auto&& pval = *pval_opt;

		auto block_memory = api.var_to_block_memory(pval);
		auto memory_view = MemoryView{ block_memory.data(), block_memory.size() };
		return std::make_optional(memory_view);
	}
	case HspObjectKind::Element:
	{
		auto&& pval_opt = path_to_pval(path.parent(), depth, api);
		if (!pval_opt) {
			return std::nullopt;
		}

		auto aptr = api.var_element_to_aptr(*pval_opt, path.as_element().indexes());
		auto block_memory = api.var_element_to_block_memory(*pval_opt, aptr);
		auto memory_view = MemoryView{ block_memory.data(), block_memory.size() };
		return std::make_optional(memory_view);
	}
	case HspObjectKind::CallFrame:
	{
		auto&& param_stack_opt = path_to_param_stack(path, MIN_DEPTH, api);
		if (!param_stack_opt || !param_stack_opt->safety()) {
			return std::nullopt;
		}

		return std::make_optional(param_stack_to_memory_view(*param_stack_opt));
	}
	default:
		return std::nullopt;
	}
}

// -----------------------------------------------
// HspObjects
// -----------------------------------------------

HspObjects::HspObjects(HspDebugApi& api, HspLogger& logger, HspScripts& scripts, HspStaticVars& static_vars, DebugInfo const& debug_info, hpiutil::DInfo const& debug_segment)
	: api_(api)
	, logger_(logger)
	, scripts_(scripts)
	, static_vars_(static_vars)
	, debug_segment_(debug_segment)
	, root_path_(std::make_shared<HspObjectPath::Root>())
	, modules_(group_vars_by_module(static_vars.get_all_names()))
	, types_(create_type_datas())
	, general_content_(create_general_content(debug_info))
{
}

auto HspObjects::root_path() const->HspObjectPath::Root const& {
	return root_path_->as_root();
}

auto HspObjects::path_to_memory_view(HspObjectPath const& path) const->std::optional<MemoryView> {
	return (::path_to_memory_view(path, MIN_DEPTH, api_));
}

auto HspObjects::type_to_name(HspType type) const->Utf8StringView {
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

auto HspObjects::module_to_name(std::size_t module_id) const -> Utf8StringView {
	return modules_.at(module_id).name();
}

auto HspObjects::module_to_var_count(std::size_t module_id) const->std::size_t {
	return modules_.at(module_id).var_ids().size();
}

auto HspObjects::module_to_var_at(std::size_t module_id, std::size_t index) const->std::size_t {
	return modules_.at(module_id).var_ids().at(index);
}

auto HspObjects::static_var_path_to_name(HspObjectPath::StaticVar const& path)->Utf8String {
	auto&& name_opt = api_.static_var_find_name(path.static_var_id());
	if (!name_opt) {
		assert(false && u8"静的変数の名前が見つかるはず");
		return to_owned(as_utf8(u8"?"));
	}

	return to_utf8(as_hsp(*std::move(name_opt)));
}

bool HspObjects::static_var_path_is_array(HspObjectPath::StaticVar const& path) {
	return api_.var_is_array(api_.static_var_to_pval(path.static_var_id()));
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
	return var_path_to_metadata(path, api_).value_or(HspVarMetadata::none());
}

auto HspObjects::element_path_to_child_count(HspObjectPath::Element const& path) const -> std::size_t {
	auto&& pval_opt = path_to_pval(path, MIN_DEPTH, api_);
	if (!pval_opt) {
		return 0;
	}

	return 1;
}

auto HspObjects::element_path_to_child_at(HspObjectPath::Element const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < element_path_to_child_count(path));

	auto&& pval_opt = path_to_pval(path, MIN_DEPTH, api_);
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

auto HspObjects::element_path_to_name(HspObjectPath::Element const& path) const -> Utf8String {
	auto v = std::vector<int>{};
	for (auto i : path.indexes()) {
		v.push_back((int)i);
	}
	auto name = hpiutil::stringifyArrayIndex(v);

	return ascii_as_utf8(std::move(name));
}

auto HspObjects::param_path_to_child_count(HspObjectPath::Param const& path) const -> std::size_t {
	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
	case MPTYPE_ARRAYVAR:
		return var_path_to_child_count(path, api_);

	case MPTYPE_SINGLEVAR:
		return 1;

	case MPTYPE_LABEL:
	case MPTYPE_LOCALSTRING:
	case MPTYPE_DNUM:
	case MPTYPE_INUM:
		return 1;

	default:
		// FIXME: 他の種類の引数に対応する (label, var, array, modvar)
		return 0;
	}
}

auto HspObjects::param_path_to_child_at(HspObjectPath::Param const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < param_path_to_child_count(path));

	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
	case MPTYPE_ARRAYVAR:
		return var_path_to_child_at(path, child_index, api_);

	case MPTYPE_SINGLEVAR:
		{
			auto param_data_opt = param_path_to_param_data(path, MIN_DEPTH, api_);
			if (!param_data_opt) {
				return path.new_unavailable(to_owned(as_utf8(u8"引数データを取得できません")));
			}

			auto var_data = api_.param_data_to_single_var(*param_data_opt);
			auto pval = api_.mp_var_data_to_pval(var_data);
			auto aptr = api_.mp_var_data_to_aptr(var_data);
			if (aptr >= api_.var_to_element_count(pval)) {
				return path.new_unavailable(to_owned(as_utf8(u8"引数に渡された要素が存在しません")));
			}

			auto indexes = api_.var_element_to_indexes(pval, aptr);
			return path.new_element(indexes);
		}

	case MPTYPE_LABEL:
		return path.new_label();

	case MPTYPE_LOCALSTRING:
		return path.new_str();

	case MPTYPE_DNUM:
		return path.new_double();

	case MPTYPE_INUM:
		return path.new_int();

	default:
		assert(false && u8"Invalid param path child index");
		throw new std::out_of_range{ u8"child_index" };
	}
}

auto HspObjects::param_path_to_name(HspObjectPath::Param const& path) const -> Utf8String {
	auto&& param_data_opt = param_path_to_param_data(path, MIN_DEPTH, api_);
	if (!param_data_opt) {
		return to_owned(as_utf8(u8"<unavailable>"));
	}

	auto&& name = api_.param_to_name(param_data_opt->param(), param_data_opt->param_index(), debug_segment_);
	return to_utf8(as_hsp(std::move(name)));
}

auto HspObjects::param_path_to_var_metadata(HspObjectPath::Param const& path) const->std::optional<HspVarMetadata> {
	// FIXME: var/modvar 引数なら指定された要素に関するメモリダンプを表示したい (要素数 1、メモリダンプはその要素の範囲のみ)
	return var_path_to_metadata(path, api_);
}

bool HspObjects::label_path_is_null(HspObjectPath::Label const& path) const {
	auto&& label_opt = label_path_to_value(path, api_);
	if (!label_opt) {
		return true;
	}

	return *label_opt == nullptr;
}

auto HspObjects::label_path_to_static_label_name(HspObjectPath::Label const& path) const -> std::optional<Utf8String> {
	auto&& static_label_id_opt = label_path_to_static_label_id(path);
	if (!static_label_id_opt) {
		return std::nullopt;
	}

	auto&& name = debug_segment_.tryFindLabelName((int)*static_label_id_opt);
	if (!name) {
		return std::nullopt;
	}

	// FIXME: 効率化 (文字列の参照かビューを返す)
	return std::make_optional(to_utf8(as_hsp(name)));
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

auto HspObjects::str_path_to_value(HspObjectPath::Str const& path) const -> Utf8String {
	static auto empty = ascii_to_utf8(u8"");

	return (::str_path_to_value(path, api_)).value_or(empty);
}

auto HspObjects::double_path_to_value(HspObjectPath::Double const& path) const->HspDouble {
	return (::double_path_to_value(path, api_)).value_or(HspDouble{});
}

auto HspObjects::int_path_to_value(HspObjectPath::Int const& path) const -> HspInt {
	return (::int_path_to_value(path, api_)).value_or(HspInt{});
}

auto HspObjects::flex_path_to_child_count(HspObjectPath::Flex const& path)->std::size_t {
	auto&& flex_opt = flex_path_to_value(path, MIN_DEPTH, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		return 0;
	}

	return api_.flex_to_member_count(*flex_opt);
}

auto HspObjects::flex_path_to_child_at(HspObjectPath::Flex const& path, std::size_t index)->std::shared_ptr<HspObjectPath const> {
	auto&& flex_opt = flex_path_to_value(path, MIN_DEPTH, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		assert(false && u8"Invalid flex path child index");
		throw new std::out_of_range{ u8"child_index" };
	}

	auto&& param_data = api_.flex_to_member_at(*flex_opt, index);
	auto param_type = api_.param_data_to_type(param_data);
	auto param_index = param_data.param_index();
	return path.new_param(param_type, param_index);
}

auto HspObjects::flex_path_is_nullmod(HspObjectPath::Flex const& path) -> std::optional<bool> {
	auto&& flex_opt = flex_path_to_value(path, MIN_DEPTH, api_);
	if (!flex_opt) {
		return std::nullopt;
	}

	return std::make_optional(api_.flex_is_nullmod(*flex_opt));
}

auto HspObjects::flex_path_is_clone(HspObjectPath::Flex const& path) -> std::optional<bool> {
	auto&& flex_opt = flex_path_to_value(path, MIN_DEPTH, api_);
	if (!flex_opt) {
		return std::nullopt;
	}

	return std::make_optional(api_.flex_is_clone(*flex_opt));
}

auto HspObjects::flex_path_to_module_name(HspObjectPath::Flex const& path) -> Utf8String {
	auto&& flex_opt = flex_path_to_value(path, MIN_DEPTH, api_);
	if (!flex_opt || api_.flex_is_nullmod(*flex_opt)) {
		return to_owned(as_utf8(u8"null"));
	}

	auto struct_dat = api_.flex_to_module_struct(*flex_opt);
	auto name = api_.struct_to_name(struct_dat);
	return to_utf8(as_hsp(name));
}

auto HspObjects::system_var_path_to_child_count(HspObjectPath::SystemVar const& path) const -> std::size_t {
	switch (path.system_var_kind()) {
	case HspSystemVarKind::Cnt:
	case HspSystemVarKind::Err:
	case HspSystemVarKind::IParam:
	case HspSystemVarKind::WParam:
	case HspSystemVarKind::LParam:
	case HspSystemVarKind::LoopLev:
	case HspSystemVarKind::SubLev:
	case HspSystemVarKind::Refstr:
	case HspSystemVarKind::Refdval:
	case HspSystemVarKind::Stat:
	case HspSystemVarKind::StrSize:
		return 1;
	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_child_at(HspObjectPath::SystemVar const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < system_var_path_to_child_count(path));

	switch (path.system_var_kind()) {
	case HspSystemVarKind::Cnt:
	case HspSystemVarKind::Err:
	case HspSystemVarKind::IParam:
	case HspSystemVarKind::WParam:
	case HspSystemVarKind::LParam:
	case HspSystemVarKind::LoopLev:
	case HspSystemVarKind::SubLev:
	case HspSystemVarKind::Stat:
	case HspSystemVarKind::StrSize:
		return path.new_int();

	case HspSystemVarKind::Refstr:
		return path.new_str();

	case HspSystemVarKind::Refdval:
		return path.new_double();

	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_name(HspObjectPath::SystemVar const& path) const -> Utf8String {
	switch (path.system_var_kind()) {
	case HspSystemVarKind::Cnt:
		return to_owned(as_utf8(u8"cnt"));

	case HspSystemVarKind::Err:
		return to_owned(as_utf8(u8"err"));

	case HspSystemVarKind::IParam:
		return to_owned(as_utf8(u8"iparam"));

	case HspSystemVarKind::WParam:
		return to_owned(as_utf8(u8"wparam"));

	case HspSystemVarKind::LParam:
		return to_owned(as_utf8(u8"lparam"));

	case HspSystemVarKind::LoopLev:
		return to_owned(as_utf8(u8"looplev"));

	case HspSystemVarKind::SubLev:
		return to_owned(as_utf8(u8"sublev"));

	case HspSystemVarKind::Refstr:
		return to_owned(as_utf8(u8"refstr"));

	case HspSystemVarKind::Refdval:
		return to_owned(as_utf8(u8"refdval"));

	case HspSystemVarKind::Stat:
		return to_owned(as_utf8(u8"stat"));

	case HspSystemVarKind::StrSize:
		return to_owned(as_utf8(u8"strsize"));

	default:
		assert(false && u8"Invalid HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::call_stack_path_to_call_frame_count(HspObjectPath::CallStack const& path) const -> std::size_t {
	return wc_call_frame_count();
}

auto HspObjects::call_stack_path_to_call_frame_id_at(HspObjectPath::CallStack const& path, std::size_t call_frame_index) const -> std::optional<std::size_t> {
	return wc_call_frame_id_at(call_frame_index);
}

auto HspObjects::call_frame_path_to_name(HspObjectPath::CallFrame const& path) const -> std::optional<Utf8String> {
	auto&& call_frame_opt = wc_call_frame_get(path.call_frame_id());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	auto struct_dat = call_frame_opt->get().struct_dat();
	auto name = hpiutil::STRUCTDAT_name(struct_dat);
	return to_utf8(as_hsp(name));
}

auto HspObjects::call_frame_path_to_child_count(HspObjectPath::CallFrame const& path) const -> std::size_t {
	auto&& param_stack_opt = path_to_param_stack(path, MIN_DEPTH, api_);
	if (!param_stack_opt) {
		return 0;
	}

	return api_.param_stack_to_data_count(*param_stack_opt);
}

auto HspObjects::call_frame_path_to_child_at(HspObjectPath::CallFrame const& path, std::size_t child_index) const -> std::optional<std::shared_ptr<HspObjectPath const>> {
	auto&& param_stack_opt = path_to_param_stack(path, MIN_DEPTH, api_);
	if (!param_stack_opt) {
		return std::nullopt;
	}

	auto&& param_data = api_.param_stack_to_data_at(*param_stack_opt, child_index);
	auto param_type = api_.param_data_to_type(param_data);
	return std::make_optional(path.new_param(param_type, param_data.param_index()));
}

auto HspObjects::call_frame_path_to_signature(HspObjectPath::CallFrame const& path) const->std::optional<std::vector<Utf8StringView>> {
	auto&& call_frame_opt = wc_call_frame_get(path.call_frame_id());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	// FIXME: debug api から取得する
	auto&& params = hpiutil::STRUCTDAT_params(call_frame_opt->get().struct_dat());

	auto names = std::vector<Utf8StringView>{};
	for (auto&& param : params) {
		names.emplace_back(as_utf8(api_.param_type_to_name(param.mptype)));
	}

	return std::make_optional(std::move(names));
}

auto HspObjects::call_frame_path_to_file_ref_name(HspObjectPath::CallFrame const& path) const -> std::optional<Utf8String> {
	auto&& call_frame_opt = wc_call_frame_get(path.call_frame_id());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	return std::make_optional(as_utf8(call_frame_opt->get().file_ref_name()));
}

auto HspObjects::call_frame_path_to_line_index(HspObjectPath::CallFrame const& path) const -> std::optional<std::size_t> {
	auto&& call_frame_opt = wc_call_frame_get(path.call_frame_id());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	return std::make_optional(call_frame_opt->get().line_index());
}

auto HspObjects::call_frame_path_to_memory_view(HspObjectPath::CallFrame const& path) const->std::optional<MemoryView> {
	auto&& param_stack_opt = path_to_param_stack(path, MIN_DEPTH, api_);
	if (!param_stack_opt) {
		return std::nullopt;
	}

	return std::make_optional(param_stack_to_memory_view(*param_stack_opt));
}

auto HspObjects::general_to_content() const -> Utf8StringView {
	return general_content_;
}

auto HspObjects::log_to_content() const -> Utf8StringView {
	return logger_.content();
}

void HspObjects::log_do_append(Utf8StringView const& text) {
	logger_.append(text);
}

void HspObjects::log_do_clear() {
	logger_.clear();
}

auto HspObjects::script_to_content() const -> Utf8StringView {
	auto file_ref_name = api_.current_file_ref_name().value_or("");
	return scripts_.content(file_ref_name);
}

auto HspObjects::script_to_current_line() const -> std::size_t {
	return api_.current_line();
}

auto HspObjects::script_to_current_location_summary() const -> Utf8String {
	// FIXME: 長すぎるときは切る
	auto file_ref_name = api_.current_file_ref_name().value_or("???");
	auto line_index = script_to_current_line();
	auto line = scripts_.line(file_ref_name, line_index).value_or(to_owned(as_utf8("???")));

	auto text = std::stringstream{};
	text << "#" << (line_index + 1) << " " << file_ref_name << "\r\n";
	text << as_native(line);
	return as_utf8(text.str());
}

// -----------------------------------------------
// HspObjects::Module
// -----------------------------------------------

HspObjects::Module::Module(Utf8String&& name)
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

HspObjects::TypeData::TypeData(Utf8String&& name)
	: name_(std::move(name))
{
}
