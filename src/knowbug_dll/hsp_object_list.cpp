#include "pch.h"

#include "../hspsdk/hsp3debug.h"
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/hsp_object_path.h"
#include "../knowbug_core/hsp_object_writer.h"
#include "../knowbug_core/hsp_objects.h"
#include "../knowbug_core/hsp_wrap_call.h"
#include "../knowbug_core/platform.h"
#include "../knowbug_core/source_files.h"
#include "../knowbug_core/step_controller.h"
#include "../knowbug_core/string_writer.h"
#include "../knowbug_core/platform.h"
#include "./hsp_object_list.h"

class HspObjectList {
	std::vector<HspObjectListItem> items_;

public:
	auto items() const ->std::vector<HspObjectListItem> const& {
		return items_;
	}

	auto size() const -> std::size_t {
		return items().size();
	}

	auto operator[](std::size_t index) const -> HspObjectListItem const& {
		return items().at(index);
	}

	auto find_by_object_id(std::size_t object_id) const -> std::optional<HspObjectListItem const*> {
		for (auto&& item : items()) {
			if (item.object_id == object_id) {
				return &item;
			}
		}
		return std::nullopt;
	}

	void add_item(HspObjectListItem item) {
		items_.push_back(std::move(item));
	}
};

static auto kind_to_string(HspObjectListDeltaKind kind) -> std::u8string_view {
	switch (kind) {
	case HspObjectListDeltaKind::Insert: return u8"insert";
	case HspObjectListDeltaKind::Remove: return u8"remove";
	case HspObjectListDeltaKind::Update: return u8"update";
	default: throw std::exception{};
	}
}

auto HspObjectListDelta::kind_name() const -> std::u8string_view {
	return kind_to_string(kind);
}

// オブジェクトリストを構築する関数。
class HspObjectListWriter {
	HspObjects& objects_;
	HspObjectList& object_list_;
	//HspObjectIdProvider& id_provider_;
	//HspObjectListExpansion& expansion_;
	HspObjectListEntity& id_provider_;
	HspObjectListEntity& expansion_;

	std::size_t depth_;

public:
	HspObjectListWriter(HspObjects& objects, HspObjectList& object_list, /*HspObjectIdProvider& id_provider, HspObjectListExpansion& expansion*/ HspObjectListEntity& id_provider, HspObjectListEntity& expansion)
		: objects_(objects)
		, object_list_(object_list)
		//, id_provider_(id_provider)
		//, expansion_(expansion)
		, id_provider_(id_provider)
		, expansion_(expansion)
		, depth_()
	{
	}

	void add(HspObjectPath const& path) {
		if (path.kind() == HspObjectKind::Ellipsis) {
			add_value(path, path);
			return;
		}

		if (path.visual_child_count(objects()) == 1) {
			auto value_path_opt = path.visual_child_at(0, objects());
			assert(value_path_opt);

			if (value_path_opt) {
				switch ((**value_path_opt).kind()) {
				case HspObjectKind::Label:
				case HspObjectKind::Str:
				case HspObjectKind::Double:
				case HspObjectKind::Int:
				case HspObjectKind::Unknown:
					add_value(path, **value_path_opt);
					return;

				default:
					break;
				}
			}
		}

		add_scope(path);
	}

	void add_children(HspObjectPath const& path) {
		if (!expansion_.is_expanded(path)) {
			return;
		}

		auto item_count = path.visual_child_count(objects());
		for (auto i = std::size_t{}; i < item_count; i++) {
			auto item_path_opt = path.visual_child_at(i, objects());
			if (!item_path_opt) {
				assert(false);
				continue;
			}

			add(**item_path_opt);
		}
	}

private:
	void add_scope(HspObjectPath const& path) {
		auto name = path.name(objects());
		auto item_count = path.visual_child_count(objects());

		auto value = std::u8string{ u8"(" };
		value += as_utf8(std::to_string(item_count));
		value += u8"):";

		auto object_id = id_provider_.path_to_object_id(path);
		object_list_.add_item(HspObjectListItem{ object_id, depth_, name, value, item_count });
		depth_++;
		add_children(path);
		depth_--;
	}

	void add_value(HspObjectPath const& path, HspObjectPath const& value_path) {
		auto name = path.name(objects());

		auto value_writer = StringWriter{};
		HspObjectWriter{ objects(), value_writer }.write_flow_form(value_path);
		auto value = value_writer.finish();

		auto object_id = id_provider_.path_to_object_id(path);
		object_list_.add_item(HspObjectListItem{ object_id, depth_, name, value, 0 });
	}

	auto objects() -> HspObjects& {
		return objects_;
	}
};

static auto diff_object_list(HspObjectList const& source, HspObjectList const& target, std::vector<HspObjectListDelta>& diff) {
	auto source_done = std::vector<bool>{};
	source_done.resize(source.size());

	auto target_done = std::vector<bool>{};
	target_done.resize(target.size());

	auto push_remove = [&](std::size_t object_id, std::size_t index) {
		if (!diff.empty()) {
			auto last = diff.size() - 1;
			if (diff[last].kind == HspObjectListDeltaKind::Remove && diff[last].index == index) {
				diff[last].count++;
				return;
			}
		}

		diff.push_back(HspObjectListDelta::new_remove(object_id, index, 1));
	};

	// FIXME: 高速化
	for (auto si = std::size_t{}; si < source.size(); si++) {
		if (source_done[si]) {
			continue;
		}

		for (auto ti = std::size_t{}; ti < target.size(); ti++) {
			if (target_done[ti]) {
				continue;
			}

			if (source[si].object_id == target[ti].object_id) {
				source_done[si] = true;
				target_done[ti] = true;
				break;
			}
		}
	}

	{
		auto si = std::size_t{};
		auto ti = std::size_t{};

		while (si < source.size() || ti < target.size()) {
			if (ti == target.size() || (si < source.size() && !source_done[si])) {
				push_remove(source[si].object_id, ti);
				si++;
				continue;
			}

			if (si == source.size() || (ti < target.size() && !target_done[ti])) {
				diff.push_back(HspObjectListDelta::new_insert(ti, target[ti]));
				ti++;
				continue;
			}

			assert(si < source.size() && ti < target.size());
			assert(source_done[si] && target_done[ti]);

			if (source[si].object_id == target[ti].object_id) {
				auto const& s = source[si];
				auto const& t = target[ti];
				if (!s.equals(t)) {
					diff.push_back(HspObjectListDelta::new_update(ti, target[ti]));
				}

				si++;
				ti++;
				continue;
			}

			assert(false && u8"パスの順番が入れ替わるケースは未実装。");
			diff.clear();
			break;
		}
	}
}

class HspObjectListEntityImpl
	: public HspObjectListEntity
{
	HspObjectList object_list_;

	std::size_t last_id_;
	std::unordered_map<std::size_t, std::shared_ptr<HspObjectPath const>> id_to_paths_;
	std::unordered_map<std::shared_ptr<HspObjectPath const>, std::size_t> path_to_ids_;

	std::unordered_map<std::shared_ptr<HspObjectPath const>, bool> expanded_;

public:
	HspObjectListEntityImpl()
		: object_list_()
		, last_id_()
		, id_to_paths_()
		, path_to_ids_()
		, expanded_()
	{
	}

	auto size() const -> std::size_t {
		return object_list_.size();
	}

	auto path_to_object_id(HspObjectPath const& path) -> std::size_t override {
		auto iter = path_to_ids_.find(path.self());
		if (iter == path_to_ids_.end()) {
			auto id = ++last_id_;
			path_to_ids_[path.self()] = id;
			id_to_paths_[id] = path.self();
			return id;
		}

		return iter->second;
	}

	auto object_id_to_path(std::size_t object_id) -> std::optional<std::shared_ptr<HspObjectPath const>> override {
		auto iter = id_to_paths_.find(object_id);
		if (iter == id_to_paths_.end()) {
			return std::nullopt;
		}

		return iter->second;
	}

	auto is_expanded(HspObjectPath const& path) const -> bool {
		auto iter = expanded_.find(path.self());
		if (iter == expanded_.end()) {
			// ルートの子要素は既定で開く。
			return path.parent().kind() == HspObjectKind::Root;
		}

		return iter->second;
	}

	auto update(HspObjects& objects) -> std::vector<HspObjectListDelta> {
		auto new_list = HspObjectList{};
		HspObjectListWriter{ objects, new_list, *this, *this }.add_children(objects.root_path());

		auto diff = std::vector<HspObjectListDelta>{};
		diff_object_list(object_list_, new_list, diff);

		for (auto&& delta : diff) {
			apply_delta(delta, new_list);
		}

		object_list_ = std::move(new_list);
		return diff;
	}

	void toggle_expand(std::size_t object_id) {
		auto path_opt = object_id_to_path(object_id);
		if (!path_opt) {
			return;
		}

		auto item_opt = object_list_.find_by_object_id(object_id);
		if (!item_opt) {
			return;
		}

		// 子要素のないノードは開閉しない。
		if ((**item_opt).child_count == 0) {
			return;
		}

		expanded_[*path_opt] = !is_expanded(**path_opt);
	}

	auto expand(std::size_t object_id, bool expand) {
		auto path_opt = object_id_to_path(object_id);
		if (!path_opt) {
			return;
		}

		if (is_expanded(**path_opt) != expand) {
			toggle_expand(object_id);
		}
	}

private:
	void apply_delta(HspObjectListDelta const& delta, HspObjectList& new_list) {
		switch (delta.kind) {
		case HspObjectListDeltaKind::Remove: {
			auto object_id = delta.object_id;
			auto iter = id_to_paths_.find(object_id);
			if (iter == id_to_paths_.end()) {
				assert(false);
				return;
			}

			auto path = iter->second;
			id_to_paths_.erase(iter);

			{
				auto iter = path_to_ids_.find(path);
				if (iter != path_to_ids_.end()) {
					path_to_ids_.erase(iter);
				}
			}

			{
				auto iter = expanded_.find(path);
				if (iter != expanded_.end()) {
					expanded_.erase(iter);
				}
			}
			return;
		}
		default:
			return;
		}
	}
};

auto HspObjectListEntity::create() -> std::unique_ptr<HspObjectListEntity> {
	return std::make_unique<HspObjectListEntityImpl>();
}
