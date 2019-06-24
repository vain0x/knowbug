#include "HspObjects.h"
#include "HspObjectPath.h"

static bool kind_can_have_value(HspObjectKind kind) {
	return kind == HspObjectKind::StaticVar || kind == HspObjectKind::Element;
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
static auto g_root_name = std::string{ "<HspObjectPath::Root>" };

auto HspObjectPath::as_root() const -> HspObjectPath::Root const& {
	if (kind() != HspObjectKind::Root) {
		assert(false && u8"Casting to root");
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Root const*)this;
}

auto HspObjectPath::Root::name(HspObjects& objects) const -> std::string {
	return g_root_name;
}

auto HspObjectPath::Root::parent() const -> HspObjectPath const& {
	return *this;
}

auto HspObjectPath::Root::child_count(HspObjects& objects) const -> std::size_t {
	return 1;
}

auto HspObjectPath::Root::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	assert(index == 0);
	return new_global_module(objects);
}

auto HspObjectPath::Root::new_global_module(HspObjects& objects) const->std::shared_ptr<HspObjectPath const> {
	return new_module(objects.module_global_id());
}

// -----------------------------------------------
// モジュールパス
// -----------------------------------------------

HspObjectPath::Module::Module(std::shared_ptr<HspObjectPath const> parent, std::size_t module_id)
	: parent_(parent)
	, module_id_(module_id)
{
}

auto HspObjectPath::Module::name(HspObjects& objects) const -> std::string {
	return objects.module_to_name(module_id()).to_owned();
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
	: parent_(parent)
	, static_var_id_(static_var_id)
{
}

auto HspObjectPath::StaticVar::child_count(HspObjects& objects) const -> std::size_t {
	return objects.static_var_path_to_child_count(*this);
}

auto HspObjectPath::StaticVar::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.static_var_path_to_child_at(*this, index);
}

auto HspObjectPath::StaticVar::name(HspObjects& objects) const -> std::string {
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

HspObjectPath::Element::Element(std::shared_ptr<HspObjectPath const> parent, HspIndexes indexes)
	: parent_(parent)
	, indexes_(indexes)
{
}

auto HspObjectPath::Element::child_count(HspObjects& objects) const -> std::size_t {
	return objects.element_path_to_child_count(*this);
}

auto HspObjectPath::Element::child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	return objects.element_path_to_child_at(*this, child_index);
}

auto HspObjectPath::Element::name(HspObjects& objects) const -> std::string {
	// FIXME: 次元の判断などを objects に任せる
	auto v = std::vector<int>{};
	for (auto i : indexes()) {
		if (!v.empty() && i == 0) {
			break;
		}
		v.push_back((int)i);
	}
	return hpiutil::stringifyArrayIndex(v);
}

auto HspObjectPath::new_element(HspIndexes const& indexes) const -> std::shared_ptr<HspObjectPath const> {
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
	: parent_(parent)
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

auto HspObjectPath::Param::name(HspObjects& objects) const -> std::string {
	return objects.param_path_to_name(*this);
}

// -----------------------------------------------
// 文字列
// -----------------------------------------------

HspObjectPath::Str::Str(std::shared_ptr<HspObjectPath const> parent)
	: parent_(parent)
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

auto HspObjectPath::Str::value(HspObjects& objects) const -> HspStr {
	return objects.str_path_to_value(*this);
}

// -----------------------------------------------
// 整数
// -----------------------------------------------

HspObjectPath::Int::Int(std::shared_ptr<HspObjectPath const> parent)
	: parent_(parent)
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
	: parent_(parent)
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

	case HspObjectKind::Str:
		on_str(path.as_str());
		return;

	case HspObjectKind::Int:
		on_int(path.as_int());
		return;

	case HspObjectKind::Flex:
		on_flex(path.as_flex());
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
