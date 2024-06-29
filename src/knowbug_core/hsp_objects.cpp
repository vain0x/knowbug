#include "pch.h"
#include <sstream>
#include "hsp_wrap_call.h"
#include "hsp_objects_module_tree.h"
#include "hsp_object_path.h"
#include "hsp_objects.h"
#include "hsx.h"
#include "hsx_debug_segment.h"
#include "source_files.h"
#include "string_split.h"

// 再帰深度の初期値
static auto const MIN_DEPTH = std::size_t{};

// 再帰深度の最大値 (スタックオーバーフローを防ぐため)
static auto const MAX_DEPTH = std::size_t{ 32 };

// ビジュアルツリーの子要素数の最大値
static constexpr auto MAX_VISUAL_CHILD_COUNT = HspObjectPath::Group::MAX_CHILD_COUNT;

static auto param_path_to_param_data(HspObjectPath::Param const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<hsx::HspParamData>;

static auto const GLOBAL_MODULE_ID = std::size_t{ 0 };

static auto const MISSING_FILE_CONTENT = to_owned(u8"ファイルが見つかりません");

static auto const MISSING_FILE_LINE = to_owned(u8"???");

auto indexes_to_string(hsx::HspDimIndex const& indexes) -> std::u8string {
	auto ss = std::stringstream{};
	ss << '(';
	for (auto i = std::size_t{}; i < indexes.dim(); ++i) {
		if (i != 0) {
			ss << ", ";
		}
		ss << indexes[i];
	}
	ss << ')';
	return as_utf8(ss.str());
}

auto var_name_to_bare_ident(std::u8string_view str) -> std::u8string_view {
	auto atmark = str.find(u8'@');
	return atmark != std::u8string::npos
		? str.substr(0, atmark)
		: str;
}

// 変数をモジュールごとに分類する。
static auto group_vars_by_module(std::vector<std::u8string> const& var_names) -> std::vector<HspObjects::Module> {
	class ModuleTreeBuilder
		: public ModuleTreeListener
	{
		std::vector<HspObjects::Module>& modules_;

	public:
		ModuleTreeBuilder(std::vector<HspObjects::Module>& modules)
			: modules_(modules)
		{
		}

		void begin_module(std::u8string_view module_name) override {
			modules_.emplace_back(HspObjects::Module{ to_owned(module_name) });
		}

		void end_module() override {
		}

		void add_var(std::size_t var_id, std::u8string_view var_name) override {
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
	types.emplace_back(u8"unknown");
	types.emplace_back(u8"label");
	types.emplace_back(u8"str");
	types.emplace_back(u8"double");
	types.emplace_back(u8"int");
	types.emplace_back(u8"struct");
	types.emplace_back(u8"comobj");
	return types;
}

static auto create_general_content(HSP3DEBUG* debug) -> std::u8string {
	auto buffer = std::stringstream{};

	auto general_info = hsx::debug_to_general_info(debug);
	auto general_info_utf8 = to_utf8(as_hsp(general_info.get()));
	auto lines = StringLines{ std::u8string_view{ general_info_utf8 } }.iter();

	while (true) {
		auto key_opt = lines.next();
		if (!key_opt) {
			break;
		}

		auto value_opt = lines.next();
		if (!value_opt) {
			break;
		}

		buffer << as_native(*key_opt) << " = ";
		buffer << as_native(*value_opt) << "\r\n";
	}

	// 拡張内容の追加
	// FIXME: hsx 経由でアクセスする
	auto exinfo = debug->hspctx->exinfo2;
	if (exinfo->actscr) {
		auto bmscr = reinterpret_cast<BMSCR*>(exinfo->HspFunc_getbmscr(*exinfo->actscr));
		if (bmscr) {
			// color
			auto color_ref = COLORREF{ bmscr->color };
			buffer
				<< "color = ("
				<< (int)GetRValue(color_ref) << ", "
				<< (int)GetGValue(color_ref) << ", "
				<< (int)GetBValue(color_ref) << ")\r\n";

			// pos
			auto point = POINT{ bmscr->cx, bmscr->cy };
			buffer << "pos = (" << point.x << ", " << point.y << ")\r\n";
		}
	}

	return as_utf8(buffer.str());
}

static auto path_to_visual_child_count_default(HspObjectPath const& path, HspObjects& objects) -> std::size_t {
	auto n = path.child_count(objects);
	if (n >= HspObjectPath::Group::THRESHOLD) {
		constexpr auto M = HspObjectPath::Group::MAX_CHILD_COUNT;
		auto group_count = (n + M - 1) / M;
		if (group_count > M) {
			return M + 1; // M 個のグループと1個の省略
		}
		return group_count;
	}

	return std::min(n, MAX_VISUAL_CHILD_COUNT);
}

static auto path_to_visual_child_at_default(HspObjectPath const& path, std::size_t child_index, HspObjects& objects) -> std::optional<std::shared_ptr<HspObjectPath const>> {
	auto n = path.child_count(objects);
	if (n >= HspObjectPath::Group::THRESHOLD) {
		constexpr auto M = HspObjectPath::Group::MAX_CHILD_COUNT;
		if (child_index >= M) {
			return path.new_ellipsis(n);
		}
		return path.new_group(child_index * M);
	}

	if (child_index >= n) {
		return std::nullopt;
	}

	return path.child_at(child_index, objects);
}

static auto path_to_pval(HspObjectPath const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<PVal const*> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar:
		{
			auto static_var_id = path.as_static_var().static_var_id();
			return hsx::static_var_to_pval(static_var_id, ctx);
		}

	case HspObjectKind::Element:
		return path_to_pval(path.parent(), depth, ctx);

	case HspObjectKind::Param:
		{
			auto param_data_opt = param_path_to_param_data(path.as_param(), depth, ctx);
			if (!param_data_opt) {
				return std::nullopt;
			}
			auto param_data = *param_data_opt;

			switch (hsx::param_data_to_type(param_data)) {
			case MPTYPE_LOCALVAR:
				return hsx::param_data_to_pval(param_data);

			case MPTYPE_SINGLEVAR:
			case MPTYPE_ARRAYVAR:
				{
					auto mp_var_opt = hsx::param_data_to_mp_var(param_data);
					if (!mp_var_opt) {
						return std::nullopt;
					}

					return hsx::mp_var_to_pval(*mp_var_opt);
				}
			case MPTYPE_MODULEVAR:
			case MPTYPE_IMODULEVAR:
			case MPTYPE_TMODULEVAR: {
				auto mod_var_opt = hsx::param_data_to_mp_mod_var(param_data);
				if (!mod_var_opt) {
					return std::nullopt;
				}
				return std::make_optional(hsx::mp_mod_var_to_pval(*mod_var_opt));
			}
			default:
				return std::nullopt;
			}
		}
	default:
		return std::nullopt;
	}
}

static auto path_to_data(HspObjectPath const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<hsx::HspData> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar:
	{
		auto static_var_id = path.as_static_var().static_var_id();
		auto pval_opt = hsx::static_var_to_pval(static_var_id, ctx);
		if (!pval_opt) {
			return std::nullopt;
		}

		return hsx::pval_to_data(*pval_opt, ctx);
	}
	case HspObjectKind::Element:
		{
			auto pval_opt = path_to_pval(path.parent(), depth, ctx);
			if (!pval_opt) {
				return std::nullopt;
			}

			auto aptr_opt = hsx::element_to_aptr(*pval_opt, path.as_element().indexes());
			if (!aptr_opt) {
				return std::nullopt;
			}

			return hsx::element_to_data(*pval_opt, *aptr_opt, ctx);
		}
	case HspObjectKind::Param:
	{
		auto param_data_opt = param_path_to_param_data(path.as_param(), depth, ctx);
		if (!param_data_opt) {
			return std::nullopt;
		}

		// FIXME: 場当たり的
		if (hsx::param_to_type(param_data_opt->param()) == MPTYPE_LOCALVAR) {
			auto pval_opt = hsx::param_data_to_pval(*param_data_opt);
			if (!pval_opt) {
				return std::nullopt;
			}

			return hsx::pval_to_data(*pval_opt, ctx);
		}

		return hsx::param_data_to_data(*param_data_opt);
	}
	case HspObjectKind::SystemVar:
	{
		return hsx::system_var_to_data(path.as_system_var().system_var_kind(), ctx);
	}
	default:
		assert(false && u8"data を取得できるべき");
		return std::nullopt;
	}
}

static auto path_to_str(HspObjectPath const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<HsxStrSpan> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar: {
		auto static_var_id = path.as_static_var().static_var_id();
		auto pval_opt = hsx::static_var_to_pval(static_var_id, ctx);
		if (!pval_opt) {
			return std::nullopt;
		}

		return hsx::pval_to_str(*pval_opt, ctx);
	}
	case HspObjectKind::Element: {
		auto pval_opt = path_to_pval(path.parent(), depth, ctx);
		if (!pval_opt) {
			return std::nullopt;
		}

		auto aptr_opt = hsx::element_to_aptr(*pval_opt, path.as_element().indexes());
		if (!aptr_opt) {
			return std::nullopt;
		}

		return hsx::element_to_str(*pval_opt, *aptr_opt, ctx);
	}
	case HspObjectKind::Param: {
		auto param_data_opt = param_path_to_param_data(path.as_param(), depth, ctx);
		if (!param_data_opt) {
			return std::nullopt;
		}

		// FIXME: 場当たり的
		if (hsx::param_to_type(param_data_opt->param()) == MPTYPE_LOCALVAR) {
			auto pval_opt = hsx::param_data_to_pval(*param_data_opt);
			if (!pval_opt) {
				return std::nullopt;
			}

			return hsx::pval_to_str(*pval_opt, ctx);
		}

		return hsx::param_data_to_str(*param_data_opt);
	}
	case HspObjectKind::SystemVar: {
		if (path.as_system_var().system_var_kind() != hsx::HspSystemVarKind::Refstr) {
			return std::nullopt;
		}
		return hsx::system_var_refstr(ctx);
	}
	default:
		assert(false && u8"str を取得できるべき");
		return std::nullopt;
	}
}

static auto var_path_to_child_count(HspObjectPath const& path, HSPCTX const* ctx) -> std::size_t {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, ctx);
	if (!pval_opt) {
		return 0;
	}

	auto pval = *pval_opt;
	return hsx::pval_to_element_count(pval);
}

static auto var_path_to_child_at(HspObjectPath const& path, std::size_t child_index, HSPCTX const* ctx) -> std::shared_ptr<HspObjectPath const> {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, ctx);
	if (!pval_opt || child_index >= var_path_to_child_count(path, ctx)) {
		assert(false && u8"Invalid var path child index");
		throw new std::out_of_range{ "child_index" };
	}

	auto pval = *pval_opt;
	auto aptr = (APTR)child_index;
	auto indexes_opt = hsx::element_to_indexes(pval, aptr);
	if (!indexes_opt) {
		return path.new_unavailable(to_owned(u8"この要素は配列に含まれていません。"));
	}

	return path.new_element(*indexes_opt);
}

static auto var_path_to_visual_child_count(HspObjectPath const& path, HSPCTX const* ctx, HspObjects& objects) -> std::size_t {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, ctx);
	if (!pval_opt) {
		return 0;
	}

	// 配列でなければ要素の子要素を直接配置する。
	if (!hsx::pval_is_standard_array(*pval_opt, ctx)) {
		if (var_path_to_child_count(path, ctx) == 0) {
			return 0;
		}

		auto child_path = var_path_to_child_at(path, 0, ctx);
		return child_path->visual_child_count(objects);
	}

	return path_to_visual_child_count_default(path, objects);
}

static auto var_path_to_visual_child_at(HspObjectPath const& path, std::size_t child_index, HSPCTX const* ctx, HspObjects& objects) -> std::optional<std::shared_ptr<HspObjectPath const>> {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, ctx);
	if (!pval_opt) {
		assert(false && u8"Invalid var path child index");
		return std::nullopt;
	}

	if (!hsx::pval_is_standard_array(*pval_opt, ctx)) {
		if (var_path_to_child_count(path, ctx) == 0) {
			return std::nullopt;
		}

		auto child_path = var_path_to_child_at(path, 0, ctx);
		return child_path->visual_child_at(child_index, objects);
	}

	return path_to_visual_child_at_default(path, child_index, objects);
}

static auto var_path_to_metadata(HspObjectPath const& path, HSPCTX const* ctx) -> std::optional<hsx::HspVarMetadata> {
	auto pval_opt = path_to_pval(path, MIN_DEPTH, ctx);
	if (!pval_opt) {
		return std::nullopt;
	}
	auto pval = *pval_opt;

	auto block_memory = hsx::pval_to_memory_block(pval, ctx);

	auto metadata = hsx::HspVarMetadata{};
	metadata.type_ = hsx::pval_to_type(pval);
	metadata.mode_ = hsx::pval_to_varmode(pval);
	metadata.lengths_ = hsx::pval_to_lengths(pval);
	metadata.element_size_ = hsx::pval_to_element_count(pval);
	metadata.data_size_ = pval->size; // FIXME: hsx を使う
	metadata.block_size_ = block_memory.size();
	metadata.data_ptr_ = pval->pt;
	metadata.master_ptr_ = pval->master;
	metadata.block_ptr_ = block_memory.data();
	return metadata;
}

static auto label_path_to_value(HspObjectPath::Label const& path, HSPCTX const* ctx) -> std::optional<HsxLabel> {
	auto data_opt = path_to_data(path.parent(), MIN_DEPTH, ctx);
	if (!data_opt) {
		assert(false && u8"label の親は data を生成できるはず");
		return std::nullopt;
	}

	return hsx::data_to_label(*data_opt);
}

static auto str_path_to_value(HspObjectPath::Str const& path, HSPCTX const* ctx) -> std::optional<HsxStrSpan> {
	return path_to_str(path.parent(), MIN_DEPTH, ctx);
}

static auto double_path_to_value(HspObjectPath::Double const& path, HSPCTX const* ctx) -> std::optional<HsxDouble> {
	auto data_opt = path_to_data(path.parent(), MIN_DEPTH, ctx);
	if (!data_opt) {
		assert(false && u8"double の親は data を生成できるはず");
		return std::nullopt;
	}

	return hsx::data_to_double(*data_opt);
}

static auto int_path_to_value(HspObjectPath::Int const& path, HSPCTX const* ctx) -> std::optional<HsxInt> {
	auto data_opt = path_to_data(path.parent(), MIN_DEPTH, ctx);
	if (!data_opt) {
		assert(false && u8"int の親は data を生成できるはず");
		return std::nullopt;
	}

	return hsx::data_to_int(*data_opt);
}

static auto flex_path_to_value(HspObjectPath::Flex const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<FlexValue const*> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	auto data_opt = path_to_data(path.parent(), depth, ctx);
	if (!data_opt) {
		return std::nullopt;
	}

	return hsx::data_to_flex(*data_opt);
}

static auto path_to_param_stack(HspObjectPath const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<hsx::HspParamStack> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::Flex:
		{
			auto flex_opt = flex_path_to_value(path.as_flex(), depth, ctx);
			if (!flex_opt) {
				return std::nullopt;
			}
			return hsx::flex_to_param_stack(*flex_opt, ctx);
		}
	case HspObjectKind::CallFrame:
		return wc_call_frame_to_param_stack(path.as_call_frame().key());

	default:
		assert(false && u8"param_stack が取れるべき");
		return std::nullopt;
	}
}

static auto param_path_to_param_data(HspObjectPath::Param const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<hsx::HspParamData> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	auto const& parent = path.parent();

	auto param_stack_opt = path_to_param_stack(parent, depth, ctx);
	if (!param_stack_opt) {
		return std::nullopt;
	}

	return hsx::param_stack_to_param_data(*param_stack_opt, path.param_index(), ctx);
}

static auto param_stack_to_memory_view(hsx::HspParamStack const& param_stack) -> MemoryView {
	return MemoryView{ param_stack.ptr(), param_stack.size() };
}

static auto path_to_memory_view(HspObjectPath const& path, std::size_t depth, HSPCTX const* ctx) -> std::optional<MemoryView> {
	if (depth >= MAX_DEPTH) {
		return std::nullopt;
	}
	depth++;

	switch (path.kind()) {
	case HspObjectKind::StaticVar:
	case HspObjectKind::Param:
	{
		// FIXME: str 引数のメモリビューに対応

		auto pval_opt = path_to_pval(path, depth, ctx);
		if (!pval_opt) {
			return std::nullopt;
		}
		auto pval = *pval_opt;

		auto block_memory = hsx::pval_to_memory_block(pval, ctx);
		auto memory_view = MemoryView{ block_memory.data(), block_memory.size() };
		return std::make_optional(memory_view);
	}
	case HspObjectKind::Element:
	{
		auto pval_opt = path_to_pval(path.parent(), depth, ctx);
		if (!pval_opt) {
			return std::nullopt;
		}

		auto aptr_opt = hsx::element_to_aptr(*pval_opt, path.as_element().indexes());
		if (!aptr_opt) {
			return std::nullopt;
		}

		auto block_memory = hsx::element_to_memory_block(*pval_opt, *aptr_opt, ctx);
		auto memory_view = MemoryView{ block_memory.data(), block_memory.size() };
		return std::make_optional(memory_view);
	}
	case HspObjectKind::CallFrame:
	{
		auto param_stack_opt = path_to_param_stack(path, MIN_DEPTH, ctx);
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
// WcDebugger
// -----------------------------------------------

class WcDebuggerImpl
	: public WcDebugger
{
	HSP3DEBUG* debug_;

	SourceFileRepository& source_file_repository_;

public:
	WcDebuggerImpl(HSP3DEBUG* debug, SourceFileRepository& source_file_repository)
		: debug_(debug)
		, source_file_repository_(source_file_repository)
	{
	}

	auto current_file_id_opt() const -> std::optional<std::size_t> {
		auto file_ref_name_opt = hsx::debug_to_file_ref_name(debug_);
		if (!file_ref_name_opt) {
			return std::nullopt;
		}

		auto id_opt = source_file_repository_.file_ref_name_to_file_id(*file_ref_name_opt);
		if (!id_opt) {
			return std::nullopt;
		}

		return id_opt->id();
	}

	void get_current_location(std::optional<std::size_t>& file_id_opt, std::size_t& line_index) override {
		hsx::debug_do_update_location(debug_);

		file_id_opt = current_file_id_opt();
		line_index = hsx::debug_to_line_index(debug_);
	}
};

// -----------------------------------------------
// HspObjects
// -----------------------------------------------

HspObjects::HspObjects(HSP3DEBUG* debug, std::vector<std::u8string>&& var_names, std::vector<HspObjects::Module>&& modules, std::unordered_map<HsxLabel, std::u8string>&& label_names, std::unordered_map<STRUCTPRM const*, std::u8string>&& param_names, std::unique_ptr<SourceFileRepository>&& source_file_repository, std::shared_ptr<WcDebugger> wc_debugger)
	: debug_(debug)
	, source_file_repository_(std::move(source_file_repository))
	, root_path_(std::make_shared<HspObjectPath::Root>())
	, var_names_(std::move(var_names))
	, modules_(std::move(modules))
	, types_(create_type_datas())
	, label_names_(std::move(label_names))
	, param_names_(std::move(param_names))
	, wc_debugger_(std::move(wc_debugger))
	, log_()
{
}

void HspObjects::initialize() {
	wc_initialize(wc_debugger_);
}

auto HspObjects::root_path() const->HspObjectPath::Root const& {
	return root_path_->as_root();
}

auto HspObjects::path_to_visual_child_count(HspObjectPath const& path)->std::size_t {
	return (::path_to_visual_child_count_default)(path, *this);
}

auto HspObjects::path_to_visual_child_at(HspObjectPath const& path, std::size_t child_index)->std::optional<std::shared_ptr<HspObjectPath const>> {
	return (::path_to_visual_child_at_default)(path, child_index, *this);
}

auto HspObjects::path_to_memory_view(HspObjectPath const& path) const->std::optional<MemoryView> {
	return (::path_to_memory_view(path, MIN_DEPTH, context()));
}

auto HspObjects::type_to_name(HsxVartype type) const->std::u8string_view {
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

auto HspObjects::module_to_name(std::size_t module_id) const -> std::u8string_view {
	return modules_.at(module_id).name();
}

auto HspObjects::module_to_var_count(std::size_t module_id) const->std::size_t {
	return modules_.at(module_id).var_ids().size();
}

auto HspObjects::module_to_var_at(std::size_t module_id, std::size_t index) const->std::size_t {
	if (index >= modules_.at(module_id).var_ids().size()) {
		// FIXME: とりあえずクラッシュしないようにする。
		return 0;
	}

	return modules_.at(module_id).var_ids().at(index);
}

auto HspObjects::static_var_path_to_name(HspObjectPath::StaticVar const& path)->std::u8string {
	if (path.static_var_id() >= var_names_.size()) {
		return to_owned(u8"???");
	}

	return var_names_[path.static_var_id()];
}

bool HspObjects::static_var_path_is_array(HspObjectPath::StaticVar const& path) {
	auto pval_opt = hsx::static_var_to_pval(path.static_var_id(), context());
	if (!pval_opt) {
		assert(false && u8"静的変数の pval はとれるべき");
		return false;
	}

	return hsx::pval_is_standard_array(*pval_opt, context());
}

auto HspObjects::static_var_path_to_type(HspObjectPath::StaticVar const& path)->HsxVartype {
	auto pval_opt = hsx::static_var_to_pval(path.static_var_id(), context());
	if (!pval_opt) {
		assert(false && u8"静的変数の pval はとれるべき");
		return HSPVAR_FLAG_NONE;
	}

	return hsx::pval_to_type(*pval_opt);
}

auto HspObjects::static_var_path_to_child_count(HspObjectPath::StaticVar const& path) const->std::size_t {
	return var_path_to_child_count(path, context());
}

auto HspObjects::static_var_path_to_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index) const->std::shared_ptr<HspObjectPath const> {
	return var_path_to_child_at(path, child_index, context());
}

auto HspObjects::static_var_path_to_visual_child_count(HspObjectPath::StaticVar const& path)->std::size_t {
	return var_path_to_visual_child_count(path, context(), *this);
}

auto HspObjects::static_var_path_to_visual_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index)->std::optional<std::shared_ptr<HspObjectPath const>> {
	return var_path_to_visual_child_at(path, child_index, context(), *this);
}

auto HspObjects::static_var_path_to_metadata(HspObjectPath::StaticVar const& path) -> hsx::HspVarMetadata {
	return var_path_to_metadata(path, context()).value_or(hsx::HspVarMetadata::none());
}

auto HspObjects::element_path_is_alive(HspObjectPath::Element const& path) const -> bool {
	auto pval_opt = path_to_pval(path.parent(), MIN_DEPTH, context());
	if (!pval_opt) {
		return false;
	}

	auto aptr_opt = hsx::element_to_aptr(*pval_opt, path.indexes());
	if (!aptr_opt) {
		return false;
	}

	assert(*aptr_opt < hsx::pval_to_element_count(*pval_opt));
	return true;
}

auto HspObjects::element_path_to_child_count(HspObjectPath::Element const& path) const -> std::size_t {
	return 1;
}

auto HspObjects::element_path_to_child_at(HspObjectPath::Element const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < element_path_to_child_count(path));

	auto pval_opt = path_to_pval(path, MIN_DEPTH, context());
	if (!pval_opt) {
		assert(false && u8"Invalid element path");
		return path.new_unavailable(to_owned(u8"変数を取得できません"));
	}

	auto type = hsx::pval_to_type(*pval_opt);
	switch (type) {
	case HSPVAR_FLAG_LABEL:
		return path.new_label();

	case HSPVAR_FLAG_STR:
		return path.new_str();

	case HSPVAR_FLAG_DOUBLE:
		return path.new_double();

	case HSPVAR_FLAG_INT:
		return path.new_int();

	case HSPVAR_FLAG_STRUCT:
		return path.new_flex();

	default:
		return path.new_unknown();
	}
}

auto HspObjects::element_path_to_name(HspObjectPath::Element const& path) const -> std::u8string {
	return indexes_to_string(path.indexes());
}

auto HspObjects::param_path_to_child_count(HspObjectPath::Param const& path) const -> std::size_t {
	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
	case MPTYPE_ARRAYVAR:
		return var_path_to_child_count(path, context());

	case MPTYPE_SINGLEVAR:
	case MPTYPE_MODULEVAR:
	case MPTYPE_IMODULEVAR:
	case MPTYPE_TMODULEVAR:
	case MPTYPE_LABEL:
	case MPTYPE_LOCALSTRING:
	case MPTYPE_DNUM:
	case MPTYPE_INUM:
		return 1;

	default:
		return 0;
	}
}

auto HspObjects::param_path_to_child_at(HspObjectPath::Param const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < param_path_to_child_count(path));

	switch (path.param_type()) {
	case MPTYPE_LOCALVAR:
	case MPTYPE_ARRAYVAR:
		return var_path_to_child_at(path, child_index, context());

	case MPTYPE_SINGLEVAR:
		{
			auto param_data_opt = param_path_to_param_data(path, MIN_DEPTH, context());
			if (!param_data_opt) {
				return path.new_unavailable(to_owned(u8"引数データを取得できません"));
			}

			auto mp_var_opt = hsx::param_data_to_mp_var(*param_data_opt);
			auto pval = hsx::mp_var_to_pval(*mp_var_opt);
			auto aptr = hsx::mp_var_to_aptr(*mp_var_opt);

			auto indexes_opt = hsx::element_to_indexes(pval, aptr);
			if (!indexes_opt) {
				return path.new_unavailable(to_owned(u8"引数に渡された要素が存在しません"));
			}

			return path.new_element(*indexes_opt);
		}
	case MPTYPE_MODULEVAR:
	case MPTYPE_IMODULEVAR:
	case MPTYPE_TMODULEVAR: {
		auto param_data_opt = param_path_to_param_data(path, MIN_DEPTH, context());
		if (!param_data_opt) {
			return path.new_unavailable(to_owned(u8"引数データを取得できません"));
		}

		auto mod_var_data_opt = hsx::param_data_to_mp_mod_var(*param_data_opt);
		if (!mod_var_data_opt) {
			return path.new_unavailable(to_owned(u8"引数データを取得できません"));
		}

		auto pval = hsx::mp_mod_var_to_pval(*mod_var_data_opt);
		auto aptr = hsx::mp_mod_var_to_aptr(*mod_var_data_opt);

		auto indexes_opt = hsx::element_to_indexes(pval, aptr);
		if (!indexes_opt) {
			return path.new_unavailable(to_owned(u8"引数に渡された要素が存在しません"));
		}

		return path.new_element(*indexes_opt);
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
		throw new std::out_of_range{ "child_index" };
	}
}

auto HspObjects::param_path_to_name(HspObjectPath::Param const& path) const -> std::u8string {
	auto param_data_opt = param_path_to_param_data(path, MIN_DEPTH, context());
	if (!param_data_opt) {
		return to_owned(u8"<unavailable>");
	}

	auto iter = param_names_.find(param_data_opt->param());
	if (iter != param_names_.end()) {
		return iter->second;
	}

	auto type = hsx::param_to_type(param_data_opt->param());
	if (type == MPTYPE_MODULEVAR || type == MPTYPE_IMODULEVAR || type == MPTYPE_TMODULEVAR) {
		return to_owned(u8"thismod");
	}

	return indexes_to_string(hsx::HspDimIndex{ 1, { param_data_opt->param_index() } });
}

auto HspObjects::param_path_to_var_metadata(HspObjectPath::Param const& path) const->std::optional<hsx::HspVarMetadata> {
	// FIXME: var/modvar 引数なら指定された要素に関するメモリダンプを表示したい (要素数 1、メモリダンプはその要素の範囲のみ)
	return var_path_to_metadata(path, context());
}

bool HspObjects::label_path_is_null(HspObjectPath::Label const& path) const {
	auto label_opt = label_path_to_value(path, context());
	if (!label_opt) {
		return true;
	}

	return *label_opt == nullptr;
}

auto HspObjects::label_path_to_static_label_name(HspObjectPath::Label const& path) const -> std::optional<std::u8string> {
	auto label_opt = label_path_to_value(path, context());
	if (!label_opt) {
		return std::nullopt;
	}

	auto iter = label_names_.find(*label_opt);
	if (iter == label_names_.end()) {
		return std::nullopt;
	}

	return iter->second;
}

auto HspObjects::label_path_to_static_label_id(HspObjectPath::Label const& path) const -> std::optional<std::size_t> {
	auto label_opt = label_path_to_value(path, context());
	if (!label_opt) {
		return std::nullopt;
	}

	// FIXME: 効率化 (事前にハッシュテーブルをつくる)
	for (auto id = std::size_t{}; id < hsx::object_temp_count(context()); id++) {
		auto opt = hsx::object_temp_to_label(id, context());
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

auto HspObjects::str_path_to_value(HspObjectPath::Str const& path) const -> HsxStrSpan {
	return (::str_path_to_value(path, context())).value_or(HsxStrSpan{ "", 0 });
}

auto HspObjects::double_path_to_value(HspObjectPath::Double const& path) const -> HsxDouble {
	return (::double_path_to_value(path, context())).value_or(0.0);
}

auto HspObjects::int_path_to_value(HspObjectPath::Int const& path) const -> HsxInt {
	return (::int_path_to_value(path, context())).value_or(0);
}

auto HspObjects::flex_path_to_child_count(HspObjectPath::Flex const& path)->std::size_t {
	auto flex_opt = flex_path_to_value(path, MIN_DEPTH, context());
	if (!flex_opt || hsx::flex_is_nullmod(*flex_opt)) {
		return 0;
	}

	return hsx::flex_to_member_count(*flex_opt, context());
}

auto HspObjects::flex_path_to_child_at(HspObjectPath::Flex const& path, std::size_t index)->std::shared_ptr<HspObjectPath const> {
	auto flex_opt = flex_path_to_value(path, MIN_DEPTH, context());
	if (!flex_opt || hsx::flex_is_nullmod(*flex_opt)) {
		assert(false && u8"Invalid flex path child index");
		throw new std::out_of_range{ "child_index" };
	}

	auto param_data_opt = hsx::flex_to_member(*flex_opt, index, context());
	if (!param_data_opt) {
		return path.new_unavailable(to_owned(u8"引数データを取得できません"));
	}

	auto param_type = hsx::param_data_to_type(*param_data_opt);
	auto param_index = param_data_opt->param_index();
	return path.new_param(param_type, param_index);
}

auto HspObjects::flex_path_is_nullmod(HspObjectPath::Flex const& path) -> std::optional<bool> {
	auto flex_opt = flex_path_to_value(path, MIN_DEPTH, context());
	if (!flex_opt) {
		return std::nullopt;
	}

	return std::make_optional(hsx::flex_is_nullmod(*flex_opt));
}

auto HspObjects::flex_path_is_clone(HspObjectPath::Flex const& path) -> std::optional<bool> {
	auto flex_opt = flex_path_to_value(path, MIN_DEPTH, context());
	if (!flex_opt) {
		return std::nullopt;
	}

	return std::make_optional(hsx::flex_is_clone(*flex_opt));
}

auto HspObjects::flex_path_to_module_name(HspObjectPath::Flex const& path) -> std::u8string {
	auto flex_opt = flex_path_to_value(path, MIN_DEPTH, context());
	if (!flex_opt || hsx::flex_is_nullmod(*flex_opt)) {
		return to_owned(u8"null");
	}

	auto struct_opt = hsx::flex_to_struct(*flex_opt, context());
	if (!struct_opt) {
		assert(false && u8"フレックスのモジュールは取得できるはず");
		return to_owned(u8"???");
	}

	auto name_opt = hsx::struct_to_name(*struct_opt, context());
	if (!name_opt) {
		assert(false && u8"モジュールの名前を取得できるはず");
		return to_owned(u8"???");
	}

	return to_utf8(as_hsp(*name_opt));
}

auto HspObjects::system_var_path_to_child_count(HspObjectPath::SystemVar const& path) const -> std::size_t {
	switch (path.system_var_kind()) {
	case hsx::HspSystemVarKind::Cnt:
	case hsx::HspSystemVarKind::Err:
	case hsx::HspSystemVarKind::IParam:
	case hsx::HspSystemVarKind::WParam:
	case hsx::HspSystemVarKind::LParam:
	case hsx::HspSystemVarKind::LoopLev:
	case hsx::HspSystemVarKind::SubLev:
	case hsx::HspSystemVarKind::Refstr:
	case hsx::HspSystemVarKind::Refdval:
	case hsx::HspSystemVarKind::Stat:
	case hsx::HspSystemVarKind::StrSize:
	case hsx::HspSystemVarKind::Thismod:
		return 1;
	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_child_at(HspObjectPath::SystemVar const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < system_var_path_to_child_count(path));

	switch (path.system_var_kind()) {
	case hsx::HspSystemVarKind::Cnt:
	case hsx::HspSystemVarKind::Err:
	case hsx::HspSystemVarKind::IParam:
	case hsx::HspSystemVarKind::WParam:
	case hsx::HspSystemVarKind::LParam:
	case hsx::HspSystemVarKind::LoopLev:
	case hsx::HspSystemVarKind::SubLev:
	case hsx::HspSystemVarKind::Stat:
	case hsx::HspSystemVarKind::StrSize:
		return path.new_int();

	case hsx::HspSystemVarKind::Refstr:
		return path.new_str();

	case hsx::HspSystemVarKind::Refdval:
		return path.new_double();

	case hsx::HspSystemVarKind::Thismod:
		return path.new_flex();

	default:
		assert(false && u8"Unknown HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::system_var_path_to_name(HspObjectPath::SystemVar const& path) const -> std::u8string {
	switch (path.system_var_kind()) {
	case hsx::HspSystemVarKind::Cnt:
		return to_owned(u8"cnt");

	case hsx::HspSystemVarKind::Err:
		return to_owned(u8"err");

	case hsx::HspSystemVarKind::IParam:
		return to_owned(u8"iparam");

	case hsx::HspSystemVarKind::WParam:
		return to_owned((u8"wparam"));

	case hsx::HspSystemVarKind::LParam:
		return to_owned(u8"lparam");

	case hsx::HspSystemVarKind::LoopLev:
		return to_owned(u8"looplev");

	case hsx::HspSystemVarKind::SubLev:
		return to_owned(u8"sublev");

	case hsx::HspSystemVarKind::Refstr:
		return to_owned(u8"refstr");

	case hsx::HspSystemVarKind::Refdval:
		return to_owned(u8"refdval");

	case hsx::HspSystemVarKind::Stat:
		return to_owned(u8"stat");

	case hsx::HspSystemVarKind::StrSize:
		return to_owned(u8"strsize");

	case hsx::HspSystemVarKind::Thismod:
		return to_owned(u8"thismod");

	default:
		assert(false && u8"Invalid HspSystemVarKind");
		throw std::exception{};
	}
}

auto HspObjects::call_stack_path_to_call_frame_count(HspObjectPath::CallStack const& path) const -> std::size_t {
	return wc_call_frame_count();
}

auto HspObjects::call_stack_path_to_call_frame_key_at(HspObjectPath::CallStack const& path, std::size_t call_frame_index) const -> std::optional<WcCallFrameKey> {
	return wc_call_frame_key_at(call_frame_index);
}

auto HspObjects::call_frame_path_to_name(HspObjectPath::CallFrame const& path) const -> std::optional<std::u8string> {
	auto call_frame_opt = wc_call_frame_get(path.key());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	auto struct_dat = call_frame_opt->get().struct_dat();
	auto name_opt = hsx::struct_to_name(struct_dat, context());
	return to_utf8(as_hsp(name_opt.value_or("???")));
}

auto HspObjects::call_frame_path_is_alive(HspObjectPath::CallFrame const& path) const -> bool {
	return wc_call_frame_get(path.key()).has_value();
}

auto HspObjects::call_frame_path_to_child_count(HspObjectPath::CallFrame const& path) const -> std::size_t {
	auto param_stack_opt = path_to_param_stack(path, MIN_DEPTH, context());
	if (!param_stack_opt) {
		return 0;
	}

	return hsx::param_stack_to_param_data_count(*param_stack_opt);
}

auto HspObjects::call_frame_path_to_child_at(HspObjectPath::CallFrame const& path, std::size_t child_index) const -> std::optional<std::shared_ptr<HspObjectPath const>> {
	auto param_stack_opt = path_to_param_stack(path, MIN_DEPTH, context());
	if (!param_stack_opt) {
		return std::nullopt;
	}

	auto param_data_opt = hsx::param_stack_to_param_data(*param_stack_opt, child_index, context());
	if (!param_data_opt) {
		return std::nullopt;
	}

	auto param_type = hsx::param_data_to_type(*param_data_opt);
	return std::make_optional(path.new_param(param_type, param_data_opt->param_index()));
}

auto HspObjects::call_frame_path_to_signature(HspObjectPath::CallFrame const& path) const->std::optional<std::vector<std::u8string_view>> {
	auto call_frame_opt = wc_call_frame_get(path.key());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	auto params = hsx::struct_to_params(call_frame_opt->get().struct_dat(), context());

	auto names = std::vector<std::u8string_view>{};
	for (auto&& param : params) {
		auto name = hsx::param_type_to_name(hsx::param_to_type(&param)).value_or("???");
		names.emplace_back(as_utf8(name));
	}

	return std::make_optional(std::move(names));
}

auto HspObjects::call_frame_path_to_full_path(HspObjectPath::CallFrame const& path) const -> std::optional<std::u8string_view> {
	auto call_frame_opt = wc_call_frame_get(path.key());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	auto file_id_opt = call_frame_opt->get().file_id_opt();
	if (!file_id_opt) {
		return std::nullopt;
	}
	auto file_id = SourceFileId{ *file_id_opt };

	auto full_path_opt = source_file_repository_->file_to_full_path_as_utf8(file_id);
	if (!full_path_opt) {
		return std::nullopt;
	}

	return std::make_optional(*full_path_opt);
}

auto HspObjects::call_frame_path_to_line_index(HspObjectPath::CallFrame const& path) const -> std::optional<std::size_t> {
	auto call_frame_opt = wc_call_frame_get(path.key());
	if (!call_frame_opt) {
		return std::nullopt;
	}

	return std::make_optional(call_frame_opt->get().line_index());
}

auto HspObjects::general_to_content() -> std::u8string {
	return create_general_content(debug());
}

auto HspObjects::log_to_content() const -> std::u8string_view {
	return log_;
}

void HspObjects::log_do_append(std::u8string_view text) {
	log_ += text;
}

void HspObjects::log_do_clear() {
	log_.clear();
}

auto HspObjects::script_to_full_path() const -> std::optional<OsStringView> {
	auto file_ref_name_opt = hsx::debug_to_file_ref_name(debug());
	if (!file_ref_name_opt) {
		return std::nullopt;
	}

	return source_file_repository_->file_ref_name_to_full_path(*file_ref_name_opt);
}

auto HspObjects::script_to_content() const -> std::u8string_view {
	auto file_ref_name = hsx::debug_to_file_ref_name(debug()).value_or("");
	return source_file_repository_->file_ref_name_to_content(file_ref_name).value_or(MISSING_FILE_CONTENT);
}

auto HspObjects::script_to_current_file() const -> std::optional<std::size_t> {
	auto file_ref_name = hsx::debug_to_file_ref_name(debug()).value_or("hsptmp");
	auto source_file_id_opt = source_file_repository_->file_ref_name_to_file_id(file_ref_name);
	if (!source_file_id_opt) {
		return std::nullopt;
	}

	return source_file_id_opt->id();
}

auto HspObjects::script_to_current_line() const -> std::size_t {
	return hsx::debug_to_line_index(debug());
}

auto HspObjects::script_to_current_location_summary() const -> std::u8string {
	// FIXME: 長すぎるときは切る
	auto file_ref_name = hsx::debug_to_file_ref_name(debug()).value_or("hsptmp");
	auto line_index = script_to_current_line();
	auto line = source_file_repository_->file_ref_name_to_line_at(file_ref_name, line_index).value_or(MISSING_FILE_LINE);

	auto text = std::stringstream{};
	text << "#" << (line_index + 1) << " " << file_ref_name << "\r\n";
	text << as_native(line);
	return as_utf8(text.str());
}

void HspObjects::script_do_update_location() {
	hsx::debug_do_update_location(debug());
}

auto HspObjects::source_file_to_full_path(std::size_t source_file_id) const->std::optional<std::u8string_view> {
	return source_file_repository_->file_to_full_path_as_utf8(SourceFileId{ source_file_id });
}

auto HspObjects::source_file_to_content(std::size_t source_file_id) const->std::optional<std::u8string_view> {
	return source_file_repository_->file_to_content(SourceFileId{ source_file_id });
}

auto HspObjects::context() const -> HSPCTX const* {
	return hsx::debug_to_context(debug());
}

// -----------------------------------------------
// HspObjects::Module
// -----------------------------------------------

HspObjects::Module::Module(std::u8string&& name)
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

HspObjects::TypeData::TypeData(std::u8string&& name)
	: name_(std::move(name))
{
}

// -----------------------------------------------
// HspObjectsBuilder
// -----------------------------------------------

static void read_debug_segment(HspObjectsBuilder& builder, SourceFileResolver& resolver, HSPCTX const* ctx) {
	auto reader = hsx::DebugSegmentReader{ ctx };
	while (true) {
		auto item_opt = reader.next();
		if (!item_opt) {
			break;
		}

		switch (item_opt->kind()) {
		case hsx::DebugSegmentItemKind::SourceFile:
			resolver.add_file_ref_name(std::string{ item_opt->str() });
			continue;

		case hsx::DebugSegmentItemKind::VarName:
			builder.add_var_name(item_opt->str());
			continue;

		case hsx::DebugSegmentItemKind::LabelName:
			builder.add_label_name(item_opt->num(), item_opt->str(), ctx);
			continue;

		case hsx::DebugSegmentItemKind::ParamName:
			builder.add_param_name(item_opt->num(), item_opt->str(), ctx);
			continue;

		default:
			continue;
		}
	}
}

void HspObjectsBuilder::add_var_name(char const* var_name) {
	var_names_.emplace_back(to_utf8(as_hsp(var_name)));
}

void HspObjectsBuilder::add_label_name(int ot_index, char const* label_name, HSPCTX const* ctx) {
	auto label_opt = hsx::object_temp_to_label((std::size_t)ot_index, ctx);
	if (!label_opt) {
		assert(false && u8"invalid ot_index");
		return;
	}

	label_names_.emplace(*label_opt, to_utf8(as_hsp(label_name)));
}

void HspObjectsBuilder::add_param_name(int param_index, char const* param_name, HSPCTX const* ctx) {
	auto param_opt = hsx::params(ctx).get((std::size_t)param_index);
	if (!param_opt) {
		assert(false && u8"invalid param_index");
		return;
	}

	auto name = to_utf8(as_hsp(param_name));
	name = to_owned(var_name_to_bare_ident(name));

	param_names_.emplace(*param_opt, std::move(name));
}

void HspObjectsBuilder::read_debug_segment(SourceFileResolver& resolver, HSPCTX const* ctx) {
	(::read_debug_segment)(*this, resolver, ctx);
}

auto HspObjectsBuilder::finish(HSP3DEBUG* debug, std::unique_ptr<SourceFileRepository>&& source_file_repository)->HspObjects {
	auto modules = group_vars_by_module(var_names_);
	auto wc_debugger = std::shared_ptr<WcDebugger>{ std::make_shared<WcDebuggerImpl>(debug, *source_file_repository) };
	return HspObjects{ debug, std::move(var_names_), std::move(modules), std::move(label_names_), std::move(param_names_), std::move(source_file_repository), std::move(wc_debugger) };
}
