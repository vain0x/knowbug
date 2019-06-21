#include "HspObjects.h"
#include "HspObjectPath.h"

static bool kind_can_have_value(HspObjectKind kind) {
	return kind == HspObjectKind::StaticVar;
}


HspObjectPath::~HspObjectPath() {
}

auto HspObjectPath::self() const -> std::shared_ptr<HspObjectPath const> {
	return shared_from_this();
}

// -----------------------------------------------
// ルートパス
// -----------------------------------------------

// オブジェクトルートへのパス (path 型)
static auto g_root = std::shared_ptr<HspObjectPath const>{
	std::make_shared<HspObjectPath::Root const>()
};

// オブジェクトルートの名前。使われないはずなので適当な文字列にしておく。
static auto g_root_name = std::string{ "<HspObjectPath::Root>" };

auto HspObjectPath::get_root() -> HspObjectPath::Root const& {
	return (HspObjectPath::Root const&)*g_root;
}

auto HspObjectPath::Root::name(HspObjects& objects) const -> std::string {
	return g_root_name;
}

auto HspObjectPath::Root::parent() const -> std::shared_ptr<HspObjectPath const> const& {
	return g_root;
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
	return objects.module_name(module_id()).to_owned();
}

bool HspObjectPath::Module::is_global(HspObjects& objects) const {
	return objects.module_global_id() == module_id();
}

auto HspObjectPath::Module::child_count(HspObjects& objects) const -> std::size_t {
	if (is_global(objects)) {
		// グローバルモジュール以外のモジュールと、グローバルモジュールに含まれる変数
		return objects.module_count() - 1 + objects.module_var_count(module_id());
	} else {
		// モジュールに含まれる変数
		return objects.module_var_count(module_id());
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
		auto static_var_id = objects.module_var_at(module_id(), index);
		return new_static_var(static_var_id);
	} else {
		auto static_var_id = objects.module_var_at(module_id(), index);
		return new_static_var(static_var_id);
	}
}

auto HspObjectPath::new_module(std::size_t module_id) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::Module>(self(), module_id);
}

auto HspObjectPath::as_module() const -> HspObjectPath::Module const& {
	if (kind() != HspObjectKind::Module) {
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
	// FIXME: 配列の要素数
	return 0;
}

auto HspObjectPath::StaticVar::child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> {
	// FIXME: 配列の要素へのパス
	throw new std::exception{ "out of index" };
}

auto HspObjectPath::StaticVar::name(HspObjects& objects) const -> std::string {
	return objects.static_var_name(static_var_id());
}

bool HspObjectPath::StaticVar::is_array(HspObjects& objects) const {
	return objects.static_var_is_array(static_var_id());
}

auto HspObjectPath::new_static_var(std::size_t static_var_id) const -> std::shared_ptr<HspObjectPath const> {
	return std::make_shared<HspObjectPath::StaticVar>(self(), static_var_id);
}

auto HspObjectPath::as_static_var() const -> HspObjectPath::StaticVar const& {
	if (kind() != HspObjectKind::StaticVar) {
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::StaticVar const*)this;
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
		throw new std::bad_cast{};
	}
	return *(HspObjectPath::Int const*)this;
}

auto HspObjectPath::Int::value(HspObjects& objects) const -> HspInt {
	switch (parent()->kind()) {
	case HspObjectKind::StaticVar:
		{
			auto static_var_id = parent()->as_static_var().static_var_id();
			return objects.static_var_to_int(static_var_id);
		}
	default:
		throw new std::exception{ "wrong kind" };
	}
}
