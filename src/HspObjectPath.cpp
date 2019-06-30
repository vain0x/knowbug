#include "HspObjects.h"
#include "HspObjectPath.h"

static bool kind_can_have_value(HspObjectKind kind) {
	return kind == HspObjectKind::StaticVar
		|| kind == HspObjectKind::Element
		|| kind == HspObjectKind::Param
		|| kind == HspObjectKind::SystemVar;
}

HspObjectPath::~HspObjectPath() {
}

auto HspObjectPath::self() const -> std::shared_ptr<HspObjectPath const> {
	return shared_from_this();
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

auto HspObjectPath::Root::name(HspObjects& objects) const -> Utf8String {
	return to_owned(g_root_name);
}

auto HspObjectPath::Root::parent() const -> HspObjectPath const& {
	return *this;
}

auto HspObjectPath::Root::child_count(HspObjects& objects) const -> std::size_t {
	return 5;
}

auto HspObjectPath::Root::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	assert(index < child_count(objects));
	switch (index) {
	case 0:
		return new_global_module(objects);
	case 1:
		return new_system_var_list();
	case 2:
		return new_call_stack();
	case 3:
		return new_log();
	case 4:
		return new_script();
	default:
		assert(false && u8"out of range");
		throw std::exception{};
	}
}

auto HspObjectPath::Root::new_global_module(HspObjects& objects) const->std::shared_ptr<HspObjectPath const> {
	return new_module(objects.module_global_id());
}

// -----------------------------------------------
// モジュールパス
// -----------------------------------------------

HspObjectPath::Module::Module(std::shared_ptr<HspObjectPath const> parent, std::size_t module_id)
	: parent_(std::move(parent))
	, module_id_(module_id)
{
}

auto HspObjectPath::Module::name(HspObjects& objects) const -> Utf8String {
	return to_owned(objects.module_to_name(module_id()));
}

bool HspObjectPath::Module::is_global(HspObjects& objects) const {
	return objects.module_global_id() == module_id();
}

auto HspObjectPath::Module::child_count(HspObjects& objects) const -> std::size_t {
	if (is_global(objects)) {
		// グローバルモジュール以外のモジュールと、グローバルモジュールに含まれる変数
		return objects.module_count() - 1 + objects.module_to_var_count(module_id());
	} else {
		// モジュールに含まれる変数
		return objects.module_to_var_count(module_id());
	}
}

auto HspObjectPath::Module::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	if (is_global(objects)) {
		if (index < objects.module_count() - 1) {
			auto module_id = 1 + index;
			assert(module_id < objects.module_count());
			return new_module(module_id);
		}

		index -= objects.module_count() - 1;
		auto static_var_id = objects.module_to_var_at(module_id(), index);
		return new_static_var(static_var_id);
	} else {
		auto static_var_id = objects.module_to_var_at(module_id(), index);
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

HspObjectPath::StaticVar::StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id)
	: parent_(std::move(parent))
	, static_var_id_(static_var_id)
{
}

auto HspObjectPath::StaticVar::child_count(HspObjects& objects) const -> std::size_t {
	return objects.static_var_path_to_child_count(*this);
}

auto HspObjectPath::StaticVar::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.static_var_path_to_child_at(*this, index);
}

auto HspObjectPath::StaticVar::name(HspObjects& objects) const -> Utf8String {
	return objects.static_var_path_to_name(*this);
}

bool HspObjectPath::StaticVar::is_array(HspObjects& objects) const {
	return objects.static_var_path_is_array(*this);
}

auto HspObjectPath::StaticVar::type(HspObjects& objects) const -> HspType {
	return objects.static_var_path_to_type(*this);
}

auto HspObjectPath::StaticVar::metadata(HspObjects& objects) const -> HspVarMetadata {
	return objects.static_var_path_to_metadata(*this);
}

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

HspObjectPath::Element::Element(std::shared_ptr<HspObjectPath const> parent, HspDimIndex const& indexes)
	: parent_(std::move(parent))
	, indexes_(indexes)
{
}

auto HspObjectPath::Element::child_count(HspObjects& objects) const -> std::size_t {
	return objects.element_path_to_child_count(*this);
}

auto HspObjectPath::Element::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.element_path_to_child_at(*this, child_index);
}

auto HspObjectPath::Element::name(HspObjects& objects) const -> Utf8String {
	return objects.element_path_to_name(*this);
}

auto HspObjectPath::new_element(HspDimIndex const& indexes) const -> std::shared_ptr<HspObjectPath const> {
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

HspObjectPath::Param::Param(std::shared_ptr<HspObjectPath const> parent, HspParamType param_type, std::size_t param_index)
	: parent_(std::move(parent))
	, param_type_(param_type)
	, param_index_(param_index)
{
}

auto HspObjectPath::new_param(HspParamType param_type, std::size_t param_index) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Param>(self(), param_type, param_index);
}

auto HspObjectPath::as_param() const -> HspObjectPath::Param const& {
	if (kind() != HspObjectKind::Param) {
		assert(false && u8"Casting to param");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Param const*)this;
}

auto HspObjectPath::Param::child_count(HspObjects& objects) const -> std::size_t {
	return objects.param_path_to_child_count(*this);
}

auto HspObjectPath::Param::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.param_path_to_child_at(*this, index);
}

auto HspObjectPath::Param::name(HspObjects& objects) const -> Utf8String {
	return objects.param_path_to_name(*this);
}

// -----------------------------------------------
// ラベル
// -----------------------------------------------

HspObjectPath::Label::Label(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

bool HspObjectPath::Label::is_null(HspObjects& objects) const {
	return objects.label_path_is_null(*this);
}

auto HspObjectPath::Label::static_label_name(HspObjects& objects) const -> std::optional<Utf8String> {
	return objects.label_path_to_static_label_name(*this);
}

auto HspObjectPath::Label::static_label_id(HspObjects& objects) const -> std::optional<std::size_t> {
	return objects.label_path_to_static_label_id(*this);
}

// -----------------------------------------------
// 文字列
// -----------------------------------------------

HspObjectPath::Str::Str(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

auto HspObjectPath::Str::value(HspObjects& objects) const -> Utf8String {
	return objects.str_path_to_value(*this);
}

// -----------------------------------------------
// 浮動小数点数
// -----------------------------------------------

HspObjectPath::Double::Double(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

auto HspObjectPath::Double::value(HspObjects& objects) const -> HspDouble {
	return objects.double_path_to_value(*this);
}

// -----------------------------------------------
// 整数
// -----------------------------------------------

HspObjectPath::Int::Int(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

auto HspObjectPath::Int::value(HspObjects& objects) const -> HspInt {
	return objects.int_path_to_value(*this);
}

// -----------------------------------------------
// フレックス
// -----------------------------------------------

HspObjectPath::Flex::Flex(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

auto HspObjectPath::Flex::child_count(HspObjects& objects) const -> std::size_t {
	return objects.flex_path_to_child_count(*this);
}

auto HspObjectPath::Flex::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.flex_path_to_child_at(*this, child_index);
}

bool HspObjectPath::Flex::is_nullmod(HspObjects& objects) const {
	return objects.flex_path_is_nullmod(*this);
}

auto HspObjectPath::Flex::module_name(HspObjects& objects) const -> char const* {
	return objects.flex_path_to_module_name(*this);
}

// -----------------------------------------------
// アンノウン
// -----------------------------------------------

HspObjectPath::Unknown::Unknown(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

auto HspObjectPath::new_unknown() const -> std::shared_ptr<HspObjectPath const> {
	assert(kind_can_have_value(kind()));
	return std::make_shared<HspObjectPath::Unknown>(self());
}

auto HspObjectPath::as_unknown() const -> HspObjectPath::Unknown const& {
	if (kind() != HspObjectKind::Flex) {
		assert(false && u8"Casting to unknown");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Unknown const*)this;
}

// -----------------------------------------------
// システム変数リスト
// -----------------------------------------------

static auto const s_system_var_list = std::array<HspSystemVarKind, 11>{{
	HspSystemVarKind::Cnt,
	HspSystemVarKind::Err,
	HspSystemVarKind::IParam,
	HspSystemVarKind::WParam,
	HspSystemVarKind::LParam,
	HspSystemVarKind::LoopLev,
	HspSystemVarKind::SubLev,
	HspSystemVarKind::Refstr,
	HspSystemVarKind::Refdval,
	HspSystemVarKind::Stat,
	HspSystemVarKind::StrSize,
}};

HspObjectPath::SystemVarList::SystemVarList(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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
	assert(s_system_var_list.back() == HspSystemVarKind::StrSize);
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

HspObjectPath::SystemVar::SystemVar(std::shared_ptr<HspObjectPath const> parent, HspSystemVarKind system_var_kind)
	: parent_(std::move(parent))
	, system_var_kind_(system_var_kind)
{
}

auto HspObjectPath::new_system_var(HspSystemVarKind system_var_kind) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::SystemVar>(self(), system_var_kind);
}

auto HspObjectPath::as_system_var() const -> HspObjectPath::SystemVar const& {
	if (kind() != HspObjectKind::SystemVar) {
		assert(false && u8"Casting to SystemVar");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::SystemVar const*)this;
}

auto HspObjectPath::SystemVar::child_count(HspObjects& objects) const -> std::size_t {
	return objects.system_var_path_to_child_count(*this);
}

auto HspObjectPath::SystemVar::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.system_var_path_to_child_at(*this, child_index);
}

auto HspObjectPath::SystemVar::name(HspObjects& objects) const -> Utf8String {
	return objects.system_var_path_to_name(*this);
}

// -----------------------------------------------
// コールスタック
// -----------------------------------------------

HspObjectPath::CallStack::CallStack(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

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

auto HspObjectPath::CallStack::child_count(HspObjects& objects) const -> std::size_t {
	return objects.call_stack_path_to_call_frame_count(*this);
}

auto HspObjectPath::CallStack::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	assert(child_index < child_count(objects));

	// 逆順
	auto index = child_count(objects) - 1 - child_index;

	auto&& call_frame_id_opt = objects.call_stack_path_to_call_frame_id_at(*this, index);
	if (!call_frame_id_opt) {
		assert(false && u8"コールフレームを取得できません");
		return new_unavailable(to_owned(as_utf8(u8"コールフレームを取得できません")));
	}

	return new_call_frame(*call_frame_id_opt);
}

// -----------------------------------------------
// コールフレーム
// -----------------------------------------------

HspObjectPath::CallFrame::CallFrame(std::shared_ptr<HspObjectPath const> parent, std::size_t call_frame_id)
	: parent_(std::move(parent))
	, call_frame_id_(call_frame_id)
{
}

auto HspObjectPath::new_call_frame(std::size_t call_frame_id) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::CallFrame>(self(), call_frame_id);
}

auto HspObjectPath::as_call_frame() const -> HspObjectPath::CallFrame const& {
	if (kind() != HspObjectKind::CallFrame) {
		assert(false && u8"Casting to CallFrame");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::CallFrame const*)this;
}

auto HspObjectPath::CallFrame::child_count(HspObjects& objects) const -> std::size_t {
	return objects.call_frame_path_to_child_count(*this);
}

auto HspObjectPath::CallFrame::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	auto&& child_opt = objects.call_frame_path_to_child_at(*this, child_index);
	if (!child_opt) {
		return new_unavailable(to_owned(as_utf8(u8"エラーが発生するおそれがあるため、この引数は表示されません")));
	}

	return *child_opt;
}

auto HspObjectPath::CallFrame::name(HspObjects& objects) const -> Utf8String {
	auto&& name_opt = objects.call_frame_path_to_name(*this);
	if (!name_opt) {
		return to_owned(as_utf8(u8"???"));
	}
	return *std::move(name_opt);
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

HspObjectPath::Log::Log(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

auto HspObjectPath::Log::content(HspObjects& objects) const -> Utf8StringView {
	return objects.log_to_content();
}

void HspObjectPath::Log::append(Utf8StringView const& text, HspObjects& objects) const {
	objects.log_do_append(text);
}

void HspObjectPath::Log::clear(HspObjects& objects) const {
	objects.log_do_clear();
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

HspObjectPath::Script::Script(std::shared_ptr<HspObjectPath const> parent)
	: parent_(std::move(parent))
{
}

auto HspObjectPath::Script::content(HspObjects& objects) const -> Utf8StringView {
	return objects.script_to_content();
}

// :thinking_face:
auto HspObjectPath::Script::current_line(HspObjects& objects) const -> std::size_t {
	return objects.script_to_current_line();
}

// -----------------------------------------------
// 利用不能
// -----------------------------------------------

auto HspObjectPath::new_unavailable(Utf8String&& reason) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Unavailable>(self(), std::move(reason));
}

auto HspObjectPath::as_unavailable() const -> HspObjectPath::Unavailable const& {
	if (kind() != HspObjectKind::Unavailable) {
		assert(false && u8"Casting to Unavailable");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Unavailable const*)this;
}

HspObjectPath::Unavailable::Unavailable(std::shared_ptr<HspObjectPath const> parent, Utf8String&& reason)
	: parent_(std::move(parent))
	, reason_(std::move(reason))
{
}

auto HspObjectPath::Unavailable::reason() const -> Utf8StringView {
	return reason_;
}

// -----------------------------------------------
// ビジター
// -----------------------------------------------

HspObjectPath::Visitor::Visitor(HspObjects& objects)
	: objects_(objects)
{
}

void HspObjectPath::Visitor::accept(HspObjectPath const& path) {
	switch (path.kind()) {
	case HspObjectKind::Root:
		on_root(path.as_root());
		return;

	case HspObjectKind::Module:
		on_module(path.as_module());
		return;

	case HspObjectKind::StaticVar:
		on_static_var(path.as_static_var());
		return;

	case HspObjectKind::Element:
		on_element(path.as_element());
		return;

	case HspObjectKind::Param:
		on_param(path.as_param());
		return;

	case HspObjectKind::Label:
		on_label(path.as_label());
		return;

	case HspObjectKind::Str:
		on_str(path.as_str());
		return;

	case HspObjectKind::Double:
		on_double(path.as_double());
		return;

	case HspObjectKind::Int:
		on_int(path.as_int());
		return;

	case HspObjectKind::Flex:
		on_flex(path.as_flex());
		return;

	case HspObjectKind::Unknown:
		on_unknown(path.as_unknown());
		return;

	case HspObjectKind::SystemVarList:
		on_system_var_list(path.as_system_var_list());
		return;

	case HspObjectKind::SystemVar:
		on_system_var(path.as_system_var());
		return;

	case HspObjectKind::CallStack:
		on_call_stack(path.as_call_stack());
		return;

	case HspObjectKind::CallFrame:
		on_call_frame(path.as_call_frame());
		return;

	case HspObjectKind::Log:
		on_log(path.as_log());
		return;

	case HspObjectKind::Script:
		on_script(path.as_script());
		return;

	case HspObjectKind::Unavailable:
		on_unavailable(path.as_unavailable());
		return;

	default:
		assert(false && u8"Unknown HspObjectKind");
		throw new std::exception{};
	}
}

void HspObjectPath::Visitor::accept_default(HspObjectPath const& path) {
	accept_children(path);
}

void HspObjectPath::Visitor::accept_parent(HspObjectPath const& path) {
	if (path.kind() == HspObjectKind::Root) {
		return;
	}

	accept(path.parent());
}

void HspObjectPath::Visitor::accept_children(HspObjectPath const& path) {
	auto child_count = path.child_count(objects());
	for (auto i = std::size_t{}; i < child_count; i++) {
		accept(*path.child_at(i, objects()));
	}
}
