#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "./hsp_object_path_fwd.h"

class HspObjectIdProvider {
public:
	virtual auto path_to_object_id(HspObjectPath const& path) -> std::size_t = 0;

	virtual auto object_id_to_path(std::size_t object_id) -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;
};

class HspObjectListExpansion {
public:
	virtual auto is_expanded(HspObjectPath const& path) const -> bool = 0;
};

class HspObjectListItem {
public:
	std::size_t object_id;
	std::size_t depth;
	std::u8string name;
	std::u8string value;
	std::size_t child_count;

	bool equals(HspObjectListItem const& other) const {
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
	std::size_t count; // (Remove only)
	std::size_t depth;
	std::u8string name;
	std::u8string value;

public:
	auto kind_name() const -> std::u8string_view;
	auto indented_name() const -> std::u8string;
};

class HspObjectListEntity
	: public HspObjectIdProvider
	, public HspObjectListExpansion
{
public:
	static auto create() -> std::unique_ptr<HspObjectListEntity>;

	virtual auto update(HspObjects& objects) -> std::vector<HspObjectListDelta> = 0;

	virtual void toggle_expand(std::size_t object_id) = 0;
};
