#include "pch.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include "hsp_object_path.h"
#include "hsp_objects.h"
#include "hsp_object_tree.h"

static auto const MAX_CHILD_COUNT = std::size_t{ 300 };
static auto const MAX_UPDATE_COUNT = std::size_t{ 100000 };

// オブジェクトツリーの自動更新にかかる制限
class UnfoldLimit {
	// 更新した要素数
	std::size_t count_;

	// ノードの深さ
	// フォーカスを持ったことがあるノードの深さは数えない。
	// フォーカスを持ったことがあるノードの一段下まで展開したい。
	std::size_t depth_;

public:
	UnfoldLimit()
		: count_(0)
		, depth_(0)
	{
	}

	void increment_depth() {
		depth_++;
	}

	void decrement_depth() {
		if (depth_ == 0) {
			assert(false && u8"decrement_depth が多すぎます");
			return;
		}
		depth_--;
	}

	void add_count(std::size_t count) {
		count_ += count;
	}

	auto ok() const -> bool {
		return count_ <= MAX_UPDATE_COUNT && depth_ <= 1;
	}
};

class Node {
	std::size_t parent_;
	std::shared_ptr<HspObjectPath const> path_;
	std::vector<std::size_t> children_;

	// フォーカスを持ったことがあるか？
	bool expanded_;

public:
	Node(std::size_t parent, std::shared_ptr<HspObjectPath const> path)
		: parent_(parent)
		, path_(path)
		, children_()
		, expanded_(false)
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

	auto expanded() const -> bool {
		return expanded_;
	}

	void expand() {
		expanded_ = true;
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
	std::size_t current_node_id_;

public:
	HspObjectTreeImpl(HspObjects& objects)
		: objects_(objects)
		, nodes_()
		, root_node_id_()
		, last_node_id_(0)
		, current_node_id_()
	{
		root_node_id_ = do_create_node(1, objects_.root_path().self());
		current_node_id_ = root_node_id_;
	}

	auto root_id() const -> std::size_t override {
		return root_node_id_;
	}

	auto current_node_id() const -> std::size_t override {
		return current_node_id_;
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

	void focus(std::size_t node_id, HspObjectTreeObserver& observer) override {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードにフォーカスしようとしています");
			return;
		}

		if (!node_is_alive(node_id)) {
			auto parent = nodes_.at(node_id).parent();

			remove_node(node_id, observer);
			focus(parent, observer);
			return;
		}

		update_root(observer);

		current_node_id_ = node_id;
		expand(node_id);
	}

	void focus_by_path(HspObjectPath const& path, HspObjectTreeObserver& observer) override {
		if (auto&& node_id_opt = find_by_path(path)) {
			focus(*node_id_opt, observer);
			return;
		}
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
		auto limit = UnfoldLimit{};
		update_children(root_id(), limit, observer);
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
	void update_children(std::size_t node_id, UnfoldLimit& limit, HspObjectTreeObserver& observer) {
		if (!nodes_.count(node_id)) {
			assert(false && u8"存在しないノードの子ノード更新をしようとしています");
			return;
		}

		auto&& children = nodes_.at(node_id).children();
		auto&& path = nodes_.at(node_id).path();
		auto is_expanded = nodes_.at(node_id).expanded();

		auto child_count = std::min(MAX_CHILD_COUNT, path.child_count(objects()));

		limit.add_count(child_count);
		if (!limit.ok()) {
			return;
		}

		if (children.empty()) {
			// 初回: ノードを順番通りに挿入する。

			for (auto i = std::size_t{}; i < child_count; i++) {
				auto&& child_path = path.child_at(i, objects());

				auto child_node_id = do_create_node(node_id, child_path);
				children.push_back(child_node_id);
				observer.did_create(child_node_id, HspObjectTreeInsertMode::Back);
			}

		} else {
			// 更新手順:
			// 中間への挿入・削除はインデックスの計算が大変なので、いまのところは避ける。
			// 子パスと子ノードの後ろから n 個が一致する、という最大の n を探す。
			// 後ろから n 個を除く子ノードをすべて削除して、後ろから n 個を除く子パスを先頭に挿入する。

			auto n = std::size_t{};

			while (n < std::min(children.size(), child_count)) {
				auto&& child_path = path.child_at(child_count - n - 1, objects());
				auto&& child_node = nodes_.at(children[children.size() - n - 1]);

				if (!child_path->equals(child_node.path())) {
					break;
				}

				n++;
			}

			// 削除
			for (auto i = children.size() - n; i >= 1;) {
				i--;

				remove_node(children.at(0), observer);
			}

			// 挿入
			for (auto i = child_count - n; i >= 1;) {
				i--;

				auto&& child_path = path.child_at(i, objects());

				auto child_node_id = do_create_node(node_id, child_path);
				children.insert(children.begin(), child_node_id);
				observer.did_create(child_node_id, HspObjectTreeInsertMode::Front);
			}
		}

		// 更新
		if (!is_expanded) {
			limit.increment_depth();
		}

		for (auto i = std::size_t{}; i < children.size(); i++) {
			update_children(children.at(i), limit, observer);
		}

		if (!is_expanded) {
			limit.decrement_depth();
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

	void expand(std::size_t node_id) {
		auto&& iter = nodes_.find(node_id);
		if (iter == nodes_.end()) {
			assert(false && u8"存在しないノードを展開しようとしています");
			return;
		}

		auto&& node = iter->second;
		if (node.expanded()) {
			return;
		}

		node.expand();

		if (node_id != root_id()) {
			expand(node.parent());
		}
	}
};

auto HspObjectTree::create(HspObjects& objects) -> std::unique_ptr<HspObjectTree> {
	return std::unique_ptr<HspObjectTree>{ std::make_unique<HspObjectTreeImpl>(objects) };
}
