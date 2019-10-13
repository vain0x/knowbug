#pragma once

#include <cstddef>
#include <memory>
#include <optional>

class HspObjectPath;

enum class HspObjectTreeInsertMode {
	Front,
	Back,
};

// オブジェクトツリーのノードにイベントが起こったときの処理を持つもの
class HspObjectTreeObserver {
public:
	virtual ~HspObjectTreeObserver() {
	}

	virtual void did_create(std::size_t node_id, HspObjectTreeInsertMode mode) {
	}

	virtual void will_destroy(std::size_t node_id) {
	}
};

// オブジェクトツリー
class HspObjectTree {
public:
	static auto create(HspObjects& objects) -> std::unique_ptr<HspObjectTree>;

	virtual ~HspObjectTree() {
	}

	virtual auto root_id() const -> std::size_t = 0;

	virtual auto path(std::size_t node_id) const -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;

	// 親ノードの ID を取得する。ルートノードからは取得できない。
	virtual auto parent(std::size_t node_id) const -> std::optional<std::size_t> = 0;

	// ノードをフォーカスする。
	// ノードをフォーカスするとき、それが生存しているか判定する。
	// 生存していなければ消去して、代わりに親ノードをフォーカスする。
	// 生存しているなら、子ノードのリストを更新する。そして、フォーカスされたノードのIDを返す。
	virtual auto focus(std::size_t node_id, HspObjectTreeObserver& observer) -> std::size_t = 0;

	virtual auto focus_by_path(HspObjectPath const& path, HspObjectTreeObserver& observer) -> std::size_t = 0;

	auto focus_root(HspObjectTreeObserver& observer) -> std::size_t {
		return focus(root_id(), observer);
	}
};
