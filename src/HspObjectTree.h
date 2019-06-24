#pragma once

#include <cstddef>
#include <memory>
#include <optional>

class HspObjectPath;

// オブジェクトツリーのノードにイベントが起こったときの処理を持つもの
class HspObjectTreeObserver {
public:
	virtual ~HspObjectTreeObserver() {
	}

	virtual void did_create(std::size_t node_id) {
	}

	virtual void will_destroy(std::size_t node_id) {
	}

	virtual void did_focus(std::size_t node_id) {
	}
};

// オブジェクトツリー
class HspObjectTree {
public:
	static auto create(HspObjects& objects) -> std::unique_ptr<HspObjectTree>;

	virtual ~HspObjectTree() {
	}

	virtual void subscribe(std::unique_ptr<HspObjectTreeObserver>&& observer) = 0;

	virtual auto path(std::size_t node_id) const -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;

	// 親ノードの子ノードのうち、自身より1つ上の子ノード (兄ノード?) のIDを取得する。
	// auto previous_sibling(std::size_t node_id) const -> std::optional<std::size_t>;

	// あるノードに焦点を当てる。
	// ノードへのパスが存在しなくなっていれば消去して、代わりに親ノードに焦点を当てる。
	// 存在していたら、子ノードの情報を更して、did_focus イベントを発行する。
	virtual void focus(std::size_t node_id) = 0;

	virtual void focus_root() = 0;

	virtual void focus_path(HspObjectPath const& path) = 0;
};
