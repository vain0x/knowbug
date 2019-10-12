#include "pch.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include "HspObjectPath.h"
#include "HspObjects.h"
#include "HspObjectTree.h"

static auto const MAX_CHILD_COUNT = std::size_t{ 3000 };

// FIXME: v1 時代のツリーと挙動を揃えるためのもの。
static auto object_path_has_children(HspObjectPath const& path) -> bool {
	switch (path.kind()) {
	case HspObjectKind::Root:
	case HspObjectKind::Module:
	case HspObjectKind::SystemVarList:
	case HspObjectKind::CallStack:
		return true;

	default:
		return false;
	}
}

static auto object_path_has_dynamic_children(HspObjectPath const& path) -> bool {
	switch (path.kind()) {
	case HspObjectKind::Root:
	case HspObjectKind::CallStack:
		return true;

	default:
		return false;
	}
}

class Node {
	std::size_t parent_;
	std::shared_ptr<HspObjectPath const> path_;
	std::vector<std::size_t> children_;

public:
	Node(std::size_t parent, std::shared_ptr<HspObjectPath const> path)
		: parent_(parent)
		, path_(path)
		, children_()
	{
	}

	// 親ノードのID。ただし、ルートノードなら自身のID。
	auto parent() const -> std::size_t {
		return parent_;
	}

	auto path() const -> HspObjectPath const& {
		return *path_;
	}

	auto children() -> std::vector<std::size_t>& {
		return children_;
	}

	auto children() const -> std::vector<std::size_t> const& {
		return children_;
	}
};

class HspObjectTreeImpl
	: public HspObjectTree
{
private:
	HspObjects& objects_;
	std::unordered_map<std::size_t, Node> nodes_;
	std::size_t root_node_id_;
	std::size_t last_node_id_;

public:
	HspObjectTreeImpl(HspObjects& objects)
		: objects_(objects)
		, nodes_()
		, root_node_id_(0)
		, last_node_id_(0)
	{
		root_node_id_ = do_create_node(1, objects_.root_path().self());
	}

	auto root_id() const -> std::size_t override {
		return root_node_id_;
	}

	auto path(std::size_t node_id) const -> std::optional<std::shared_ptr<HspObjectPath const>> override {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードのパスを要求しています");
			return std::nullopt;
		}

		return std::make_optional(nodes_.at(node_id).path().self());
	}

	auto parent(std::size_t node_id) const -> std::optional<std::size_t> override {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードの親ノードを探しています");
			return std::nullopt;
		}

		auto parent = nodes_.at(node_id).parent();
		if (parent == node_id) {
			return std::nullopt;
		}

		return std::make_optional(parent);
	}

	// 親ノードの子ノードのうち、自身より1つ上の子ノード (兄ノード?) のIDを取得する。
	auto previous_sibling(std::size_t node_id) const -> std::optional<std::size_t> {
		return std::nullopt;
	}

	auto focus(std::size_t node_id, HspObjectTreeObserver& observer) -> std::size_t override {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードにフォーカスしようとしています");
			return root_id();
		}

		if (!node_is_alive(node_id)) {
			auto parent = nodes_.at(node_id).parent();

			remove_node(node_id, observer);
			return focus(parent, observer);
		}

		update_root(observer);
		return node_id;
	}

	auto focus_by_path(HspObjectPath const& path, HspObjectTreeObserver& observer) -> std::size_t override {
		if (auto&& node_id_opt = find_by_path(path)) {
			return focus(*node_id_opt, observer);
		}

		return root_id();
	}

private:
	auto objects() const -> HspObjects& {
		return objects_;
	}

	auto find_by_path(HspObjectPath const& path) -> std::optional<std::size_t> {
		if (path.kind() == HspObjectKind::Root) {
			return std::make_optional(root_id());
		}

		// FIXME: 効率化？
		if (auto&& node_id_opt = find_by_path(path.parent())) {
			auto&& node = nodes_.at(*node_id_opt);
			for (auto child_id : node.children()) {
				if (nodes_.at(child_id).path().equals(path)) {
					return std::make_optional(child_id);
				}
			}
		}
		return std::nullopt;
	}

	void update_root(HspObjectTreeObserver& observer) {
		update_children(root_id(), observer);
	}

	auto do_create_node(std::size_t parent_id, std::shared_ptr<HspObjectPath const> path) -> std::size_t {
		auto node_id = ++last_node_id_;
		nodes_.emplace(node_id, Node{ parent_id, path });
		return node_id;
	}

	void remove_node(std::size_t node_id, HspObjectTreeObserver& observer) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードを削除しようとしています");
			return;
		}

		auto&& node = nodes_.at(node_id);

		// 子ノードをすべて削除する。
		{
			auto&& children = node.children();
			while (!children.empty()) {
				auto child_id = children.back();
				children.pop_back();

				remove_node(child_id, observer);
			}
		}

		// 親ノードの子ノードリストから除外する。
		if (node.parent() != node_id) {
			auto&& parent = nodes_.at(node.parent());
			auto&& siblings = parent.children();
			auto&& iter = std::find(siblings.begin(), siblings.end(), node_id);
			if (iter != siblings.end()) {
				siblings.erase(iter);
			}
		}

		observer.will_destroy(node_id);
		nodes_.erase(node_id);
	}

	// 指定したノードに対応するパスの子要素のうち、
	// 無効なパスに対応する子ノードがあれば削除し、
	// 有効なパスに対応する子ノードがなければ挿入する。
	void update_children(std::size_t node_id, HspObjectTreeObserver& observer) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードの子ノード更新をしようとしています");
			return;
		}

		auto&& children = nodes_.at(node_id).children();
		auto&& path = nodes_.at(node_id).path();

		if (!object_path_has_children(path)) {
			return;
		}

		auto child_count = path.child_count(objects());
		if (!children.empty() && !object_path_has_dynamic_children(path)) {
			return;
		}

		// FIXME: 先頭への挿入時の挙動を効率化する (コールスタックのため)

		// 手順:
		// 挿入・削除はインデックスの計算がとてもめんどうなので、いまのところは避ける。
		// 子パスと子ノードの前から n 個が一致する、という最大の n を探す。
		// 前から n 個を除く子ノードをすべて削除して、前から n 個を除く子パスを末尾に挿入する。

		auto n = std::size_t{};

		while (n < children.size() && n < child_count) {
			auto&& child_path = path.child_at(n, objects());
			auto&& child_node = nodes_.at(children[n]);

			if (!child_path->equals(child_node.path())) {
				break;
			}

			n++;
		}

		// 削除
		for (auto i = children.size(); i >= 1;) {
			i--;

			if (i < n) {
				break;
			}

			remove_node(children.at(i), observer);
		}

		// 挿入
		for (auto i = n; i < std::min(MAX_CHILD_COUNT, child_count); i++) {
			auto&& child_path = path.child_at(i, objects());

			auto child_node_id = do_create_node(node_id, child_path);
			children.push_back(child_node_id);
			observer.did_create(child_node_id);
		}

		// 更新
		for (auto i = std::size_t{}; i < children.size(); i++) {
			update_children(children.at(i), observer);
		}
	}

	bool node_is_alive(std::size_t node_id) const {
		auto&& iter = nodes_.find(node_id);
		if (iter == nodes_.end()) {
			assert(false && u8"存在しないノードの生存検査をしようとしています");
			return false;
		}

		auto&& node = iter->second;
		return node.path().is_alive(objects());
	}
};

auto HspObjectTree::create(HspObjects& objects) -> std::unique_ptr<HspObjectTree> {
	return std::unique_ptr<HspObjectTree>{ std::make_unique<HspObjectTreeImpl>(objects) };
}
