#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "./hsp_object_path_fwd.h"

// オブジェクトリスト (リストビューのデータ)

// リストの各項目はオブジェクトを指すパスに対応していて、
// そのパスが指すオブジェクトがリストの「値」列の内容になる。
// また、各項目の識別子として ID も振られる (ID とパスは 1:1 に対応)

// 階層構造: オブジェクトリスト自体に階層構造はないが、
// 項目に対応しているパスの子要素をその項目の子要素とみなす。
// 「名前」列をインデントすることで階層が分かりやすいようにしている。

// 開閉状態: リストの各項目の開閉状態は knowbug サーバー側で管理している
// 「開かれた状態」(expanded)の項目は、子要素もリストに含む。
// 一方、項目が閉じた状態なら、その子要素はスキップされて、リストに含まれない。
// (リストを構築する際に大量データを処理しなくて済むようにするため)

// リスト項目のパスと ID の対応を記録するもの
class HspObjectIdProvider {
public:
	virtual auto path_to_object_id(HspObjectPath const& path) -> std::size_t = 0;

	virtual auto object_id_to_path(std::size_t object_id) -> std::optional<std::shared_ptr<HspObjectPath const>> = 0;
};

// リスト項目の開閉状態を管理するもの
class HspObjectListExpansion {
public:
	virtual auto is_expanded(HspObjectPath const& path) const -> bool = 0;
};

// オブジェクトリストの生成結果に含まれる、リスト項目の内容
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

// リストの差分の種類
enum class HspObjectListDeltaKind {
	Insert,
	Remove,
	Update,
};

// リストの差分 (変更内容)
class HspObjectListDelta {
public:
	HspObjectListDeltaKind kind;
	std::size_t object_id;
	std::size_t index;
	std::size_t count; // (Remove only), 削除件数
	std::size_t depth;
	std::u8string name;
	std::u8string value;

public:
	auto kind_name() const -> std::u8string_view;
	auto indented_name() const -> std::u8string;
};

// オブジェクトリスト機能の状態を保持するもの
class HspObjectListEntity
	: public HspObjectIdProvider
	, public HspObjectListExpansion
{
public:
	static auto create() -> std::unique_ptr<HspObjectListEntity>;

	// オブジェクトリストを再生成し、前回との差分を出力する
	virtual auto update(HspObjects& objects) -> std::vector<HspObjectListDelta> = 0;

	// リスト項目の開閉状態をトグルする
	virtual void toggle_expand(std::size_t object_id) = 0;
};
