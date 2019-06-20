#include "HspObjects.h"
#include "HspObjectPath.h"

HspObjectPath::~HspObjectPath() {
}

auto HspObjectPath::self() const -> std::shared_ptr<HspObjectPath const> {
	return shared_from_this();
}

// -----------------------------------------------
// ルートノード
// -----------------------------------------------

// オブジェクトルートへのパス (root 型)
static auto g_root = std::make_shared<HspObjectPath::Root const>();

// オブジェクトルートへのパス (path 型)
static auto g_root_as_path = std::shared_ptr<HspObjectPath const>{ g_root };

// オブジェクトルートの名前。使われないはずなので適当な文字列にしておく。
static auto g_root_name = std::string{ "<HspObjectPath::Root>" };

auto HspObjectPath::get_root() -> HspObjectPath::Root const& {
	return *g_root;
}

auto HspObjectPath::Root::parent() const -> std::shared_ptr<HspObjectPath const> const& {
	return g_root_as_path;
}

auto HspObjectPath::Root::name(HspObjects& objects) const -> std::string {
	return g_root_name;
}

// -----------------------------------------------
// 静的変数
// -----------------------------------------------

HspObjectPath::StaticVar::StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id)
	: parent_(parent)
	, static_var_id_(static_var_id)
{
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
