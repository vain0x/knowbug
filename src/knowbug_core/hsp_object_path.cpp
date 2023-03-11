#include "pch.h"
#include "hsp_objects.h"
#include "hsp_object_path.h"

static bool kind_can_have_value(HspObjectKind kind) {
	return kind == HspObjectKind::StaticVar
		|| kind == HspObjectKind::Param
		|| kind == HspObjectKind::Element
		|| kind == HspObjectKind::Param
		|| kind == HspObjectKind::SystemVar;
}

HspObjectPath::~HspObjectPath() {
}

auto HspObjectPath::self() const -> std::shared_ptr<HspObjectPath const> {
	return shared_from_this();
}

auto HspObjectPath::hash() const -> std::size_t {
	return HashCode::from(parent().hash()).combine(kind()).combine(do_hash()).value();
}

auto HspObjectPath::visual_child_count(HspObjects& objects) const -> std::size_t {
	return objects.path_to_visual_child_count(*this);
}

auto HspObjectPath::visual_child_at(std::size_t child_index, HspObjects& objects) const -> std::optional<std::shared_ptr<HspObjectPath const>> {
	return objects.path_to_visual_child_at(*this, child_index);
}

auto HspObjectPath::memory_view(HspObjects& objects) const -> std::optional<MemoryView> {
	return objects.path_to_memory_view(*this);
}

// -----------------------------------------------
// ルートパス
// -----------------------------------------------

// オブジェクトルートの名前。使われないはずなので適当な文字列にしておく。
static auto const g_root_name = as_utf8(u8"<HspObjectPath::Root>");

auto HspObjectPath::as_root() const -> HspObjectPath::Root const& {
	if (kind() != HspObjectKind::Root) {
		assert(false && u8"Casting to root");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Root const*)this;
}

auto HspObjectPath::Root::name(HspObjects& objects) const -> std::u8string {
	return to_owned(g_root_name);
}

auto HspObjectPath::Root::child_count(HspObjects& objects) const -> std::size_t {
	return 6;
}

auto HspObjectPath::Root::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < child_count(objects));
	switch (child_index) {
	case 0:
		return new_global_module(objects);
	case 1:
		return new_system_var_list();
	case 2:
		return new_call_stack();
	case 3:
		return new_general();
	case 4:
		return new_log();
	case 5:
		return new_script();
	default:
		assert(false && u8"out of range");
		throw std::exception{};
	}
}

// -----------------------------------------------
// グループパス
// -----------------------------------------------

auto HspObjectPath::new_group(std::size_t offset) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Group>(self(), offset);
}

auto HspObjectPath::as_group() const -> HspObjectPath::Group const& {
	if (kind() != HspObjectKind::Group) {
		assert(false && u8"Casting to group");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Group const*)this;
}

auto HspObjectPath::Group::child_count(HspObjects& objects) const->std::size_t {
	auto n = parent().child_count(objects);
	return std::min(n - std::min(n, offset()), MAX_CHILD_COUNT);
}

auto HspObjectPath::Group::child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> {
	auto n = parent().child_count(objects);
	auto i = offset() + child_index;
	if (i >= n) {
		return new_unavailable(to_owned(u8"out of range"));
	}

	return parent().child_at(i, objects);
}

auto HspObjectPath::Group::name(HspObjects& objects) const->std::u8string {
	auto n = parent().child_count(objects);
	if (offset() >= n) {
		return to_owned(u8"...");
	}
	assert(n >= 1);

	auto first_index = offset();
	auto last_index = std::min(n - 1, offset() + MAX_CHILD_COUNT - 1);
	assert(first_index <= last_index);

	auto name = parent().child_at(first_index, objects)->name(objects);
	auto last = parent().child_at(last_index, objects)->name(objects);

	name += u8"...";
	name += last;
	return name;
}

// -----------------------------------------------
// 省略パス
// -----------------------------------------------

auto HspObjectPath::new_ellipsis(std::size_t total_count) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Ellipsis>(self(), total_count);
}

auto HspObjectPath::as_ellipsis() const -> HspObjectPath::Ellipsis const& {
	if (kind() != HspObjectKind::Ellipsis) {
		assert(false && u8"Casting to ellipsis");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Ellipsis const*)this;
}

auto HspObjectPath::Ellipsis::child_count(HspObjects& objects) const->std::size_t {
	return 0;
}

auto HspObjectPath::Ellipsis::child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> {
	assert(false);
	throw std::exception{};
}

auto HspObjectPath::Ellipsis::name(HspObjects& objects) const->std::u8string {
	return to_owned(u8"...");
}

// -----------------------------------------------
// モジュールパス
// -----------------------------------------------

auto HspObjectPath::Module::child_count(HspObjects& objects) const -> std::size_t {
	if (is_global(objects)) {
		// グローバルモジュール以外のモジュールと、グローバルモジュールに含まれる変数
		return objects.module_count() - 1 + objects.module_to_var_count(module_id());
	} else {
		// モジュールに含まれる変数
		return objects.module_to_var_count(module_id());
	}
}

auto HspObjectPath::Module::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	if (is_global(objects)) {
		if (child_index < objects.module_count() - 1) {
			auto module_id = 1 + child_index;
			assert(module_id < objects.module_count());
			return new_module(module_id);
		}

		child_index -= objects.module_count() - 1;
		auto static_var_id = objects.module_to_var_at(module_id(), child_index);
		return new_static_var(static_var_id);
	} else {
		auto static_var_id = objects.module_to_var_at(module_id(), child_index);
		return new_static_var(static_var_id);
	}
}

auto HspObjectPath::new_module(std::size_t module_id) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Module>(self(), module_id);
}

auto HspObjectPath::as_module() const -> HspObjectPath::Module const& {
	if (kind() != HspObjectKind::Module) {
		assert(false && u8"Casting to module");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Module const*)this;
}

// -----------------------------------------------
// 静的変数パス
// -----------------------------------------------

auto HspObjectPath::new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::StaticVar>(self(), static_var_id);
}

auto HspObjectPath::as_static_var() const -> HspObjectPath::StaticVar const& {
	if (kind() != HspObjectKind::StaticVar) {
		assert(false && u8"Casting to static var");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::StaticVar const*)this;
}

// -----------------------------------------------
// 配列要素
// -----------------------------------------------

auto HspObjectPath::new_element(hsx::HspDimIndex const& indexes) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Element>(self(), indexes);
}

auto HspObjectPath::as_element() const -> HspObjectPath::Element const& {
	if (kind() != HspObjectKind::Element) {
		assert(false && u8"Casting to element");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Element const*)this;
}

// -----------------------------------------------
// 引数
// -----------------------------------------------

auto HspObjectPath::new_param(hsx::HspParamType param_type, std::size_t param_index) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Param>(self(), param_type, param_index);
}

auto HspObjectPath::as_param() const -> HspObjectPath::Param const& {
	if (kind() != HspObjectKind::Param) {
		assert(false && u8"Casting to param");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Param const*)this;
}

// -----------------------------------------------
// ラベル
// -----------------------------------------------

auto HspObjectPath::new_label() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Label>(self());
}

auto HspObjectPath::as_label() const -> HspObjectPath::Label const& {
	if (kind() != HspObjectKind::Label) {
		assert(false && u8"Casting to label");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Label const*)this;
}

// -----------------------------------------------
// 文字列
// -----------------------------------------------

auto HspObjectPath::new_str() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Str>(self());
}

auto HspObjectPath::as_str() const -> HspObjectPath::Str const& {
	if (kind() != HspObjectKind::Str) {
		assert(false && u8"Casting to string");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Str const*)this;
}

// -----------------------------------------------
// 浮動小数点数
// -----------------------------------------------

auto HspObjectPath::new_double() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Double>(self());
}

auto HspObjectPath::as_double() const -> HspObjectPath::Double const& {
	if (kind() != HspObjectKind::Double) {
		assert(false && u8"Casting to double");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Double const*)this;
}

// -----------------------------------------------
// 整数
// -----------------------------------------------

auto HspObjectPath::new_int() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Int>(self());
}

auto HspObjectPath::as_int() const -> HspObjectPath::Int const& {
	if (kind() != HspObjectKind::Int) {
		assert(false && u8"Casting to int");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Int const*)this;
}

// -----------------------------------------------
// フレックス
// -----------------------------------------------

auto HspObjectPath::new_flex() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Flex>(self());
}

auto HspObjectPath::as_flex() const -> HspObjectPath::Flex const& {
	if (kind() != HspObjectKind::Flex) {
		assert(false && u8"Casting to flex");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Flex const*)this;
}

// -----------------------------------------------
// アンノウン
// -----------------------------------------------

auto HspObjectPath::new_unknown() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Unknown>(self());
}

auto HspObjectPath::as_unknown() const -> HspObjectPath::Unknown const& {
	if (kind() != HspObjectKind::Unknown) {
		assert(false && u8"Casting to unknown");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Unknown const*)this;
}

// -----------------------------------------------
// システム変数リスト
// -----------------------------------------------

static auto const s_system_var_list = std::array<hsx::HspSystemVarKind, 12>{ {
	hsx::HspSystemVarKind::Cnt,
	hsx::HspSystemVarKind::Err,
	hsx::HspSystemVarKind::IParam,
	hsx::HspSystemVarKind::WParam,
	hsx::HspSystemVarKind::LParam,
	hsx::HspSystemVarKind::LoopLev,
	hsx::HspSystemVarKind::SubLev,
	hsx::HspSystemVarKind::Refstr,
	hsx::HspSystemVarKind::Refdval,
	hsx::HspSystemVarKind::Stat,
	hsx::HspSystemVarKind::StrSize,
	hsx::HspSystemVarKind::Thismod,
}};

auto HspObjectPath::new_system_var_list() const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::SystemVarList>(self());
}

auto HspObjectPath::as_system_var_list() const -> HspObjectPath::SystemVarList const& {
	if (kind() != HspObjectKind::SystemVarList) {
		assert(false && u8"Casting to SystemVarList");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::SystemVarList const*)this;
}

auto HspObjectPath::SystemVarList::child_count(HspObjects& objects) const -> std::size_t {
	assert(s_system_var_list.back() == hsx::HspSystemVarKind::Thismod);
	return s_system_var_list.size();
}

auto HspObjectPath::SystemVarList::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < child_count(objects));

	auto kind = s_system_var_list.at(child_index);
	return new_system_var(kind);
}

// -----------------------------------------------
// システム変数
// -----------------------------------------------

auto HspObjectPath::new_system_var(hsx::HspSystemVarKind system_var_kind) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::SystemVar>(self(), system_var_kind);
}

auto HspObjectPath::as_system_var() const -> HspObjectPath::SystemVar const& {
	if (kind() != HspObjectKind::SystemVar) {
		assert(false && u8"Casting to SystemVar");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::SystemVar const*)this;
}

// -----------------------------------------------
// コールスタック
// -----------------------------------------------

auto HspObjectPath::new_call_stack() const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::CallStack>(self());
}

auto HspObjectPath::as_call_stack() const -> HspObjectPath::CallStack const& {
	if (kind() != HspObjectKind::CallStack) {
		assert(false && u8"Casting to CallStack");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::CallStack const*)this;
}

// -----------------------------------------------
// コールフレーム
// -----------------------------------------------

auto HspObjectPath::new_call_frame(WcCallFrameKey const& key) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::CallFrame>(self(), key);
}

auto HspObjectPath::as_call_frame() const -> HspObjectPath::CallFrame const& {
	if (kind() != HspObjectKind::CallFrame) {
		assert(false && u8"Casting to CallFrame");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::CallFrame const*)this;
}

// -----------------------------------------------
// 全般
// -----------------------------------------------

auto HspObjectPath::new_general() const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::General>(self());
}

auto HspObjectPath::as_general() const -> HspObjectPath::General const& {
	if (kind() != HspObjectKind::General) {
		assert(false && u8"Casting to General");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::General const*)this;
}

// -----------------------------------------------
// ログ
// -----------------------------------------------

auto HspObjectPath::new_log() const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Log>(self());
}

auto HspObjectPath::as_log() const -> HspObjectPath::Log const& {
	if (kind() != HspObjectKind::Log) {
		assert(false && u8"Casting to log");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Log const*)this;
}

// -----------------------------------------------
// スクリプト
// -----------------------------------------------

auto HspObjectPath::new_script() const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Script>(self());
}

auto HspObjectPath::as_script() const -> HspObjectPath::Script const& {
	if (kind() != HspObjectKind::Script) {
		assert(false && u8"Casting to script");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Script const*)this;
}

// -----------------------------------------------
// 利用不能
// -----------------------------------------------

auto HspObjectPath::new_unavailable(std::u8string&& reason) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Unavailable>(self(), std::move(reason));
}

auto HspObjectPath::as_unavailable() const -> HspObjectPath::Unavailable const& {
	if (kind() != HspObjectKind::Unavailable) {
		assert(false && u8"Casting to Unavailable");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Unavailable const*)this;
}
