#pragma once

class HspObjectPath;
class HspObjects;

class HspObjectListItem {
public:
	std::size_t object_id;
	std::size_t depth;
	std::u8string name;
	std::u8string value;
	std::size_t child_count;

	auto equals(HspObjectListItem const& other) const -> bool {
		return object_id == other.object_id
			&& depth == other.depth
			&& name == other.name
			&& value == other.value
			&& child_count == other.child_count;
	}
};

enum class HspObjectListDeltaKind {
	Insert,
	Remove,
	Update,
};

class HspObjectListDelta {
public:
	HspObjectListDeltaKind kind;
	std::size_t object_id;
	std::size_t index;
	std::size_t count;
	std::size_t depth;
	std::u8string name;
	std::u8string value;

	static auto new_insert(std::size_t index, HspObjectListItem const& item) -> HspObjectListDelta {
		return HspObjectListDelta{
			HspObjectListDeltaKind::Insert,
			item.object_id,
			index,
			0, // count
			item.depth,
			item.name,
			item.value
		};
	}

	static auto new_remove(std::size_t object_id, std::size_t index, std::size_t count) -> HspObjectListDelta {
		assert(count >= 1);
		auto delta = HspObjectListDelta{
			HspObjectListDeltaKind::Remove,
			object_id,
			index,
			count,
			0, // depth
			std::u8string{},
			std::u8string{}
		};
		return delta;
	}

	static auto new_update(std::size_t index, HspObjectListItem const& item) -> HspObjectListDelta {
		return HspObjectListDelta{
			HspObjectListDeltaKind::Update,
			item.object_id,
			index,
			0, // count
			item.depth,
			item.name,
			item.value
		};
	}

	auto kind_name() const -> std::u8string_view;

	auto indented_name() const -> std::u8string {
		static constexpr auto SPACES = u8"                ";

		auto s = to_owned(as_utf8(SPACES).substr(0, depth * 2));
		s += name;
		return s;
	}

	auto with_count(std::size_t count) const -> HspObjectListDelta {
		return HspObjectListDelta::new_remove(object_id, index, count);
	}
};

class HspObjectListEntity {
public:
	static auto create() -> std::unique_ptr<HspObjectListEntity>;

	virtual auto path_to_object_id(HspObjectPath const& path) -> std::size_t = 0;
	virtual auto object_id_to_path(std::size_t object_id) -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;

	virtual auto is_expanded(HspObjectPath const& path) const -> bool = 0;
	virtual void toggle_expand(std::size_t object_id) = 0;

	virtual auto update(HspObjects& objects) -> std::vector<HspObjectListDelta> = 0;
};
