#pragma once

#include <string>
#include <vector>
#include "encoding.h"
#include "hash_code.h"
#include "hsx.h"
#include "hsp_object_path_fwd.h"
#include "hsp_objects.h"
#include "hsp_wrap_call.h"
#include "memory_view.h"

// -----------------------------------------------
// ルートパス
// -----------------------------------------------

class HspObjectPath::Root final
	: public HspObjectPath
{
public:
	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Root;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return kind() == other.kind();
	}

	bool equals(HspObjectPath const& other) const override {
		return kind() == other.kind();
	}

	auto do_hash() const -> std::size_t override {
		return 1;
	}

	auto hash() const -> std::size_t override {
		return 1;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *this;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->std::u8string override;

	auto new_global_module(HspObjects& objects) const->std::shared_ptr<HspObjectPath const> {
		return new_module(objects.module_global_id());
	}
};

// -----------------------------------------------
// グループパス
// -----------------------------------------------

class HspObjectPath::Group final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t offset_;

public:
	// グループノードの要素数の最大値。
	static constexpr auto MAX_CHILD_COUNT = std::size_t{ 100 };

	// グループノードを挟んだときに表示される親要素の子要素の総数。
	static constexpr auto MAX_CHILD_COUNT_TOTAL = MAX_CHILD_COUNT * MAX_CHILD_COUNT;

	// これ以上の要素数を持つノードには、グループノードを挟む。
	static constexpr auto THRESHOLD = std::size_t{ 201 };

	Group(std::shared_ptr<HspObjectPath const> parent, std::size_t offset)
		: parent_(std::move(parent))
		, offset_(offset)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Group;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return offset() == other.as_group().offset();
	}

	auto do_hash() const -> std::size_t override {
		return offset();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return parent().is_alive(objects)
			&& parent().child_count(objects) >= std::min(THRESHOLD, offset() + 1);
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const->std::size_t override;

	auto child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->std::u8string override;

	auto offset() const -> std::size_t {
		return offset_;
	}
};

// -----------------------------------------------
// 省略パス
// -----------------------------------------------

class HspObjectPath::Ellipsis final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t total_count_;

public:
	Ellipsis(std::shared_ptr<HspObjectPath const> parent, std::size_t total_count)
		: parent_(std::move(parent))
		, total_count_(total_count)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Ellipsis;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return total_count() == other.as_ellipsis().total_count();
	}

	auto do_hash() const -> std::size_t override {
		return total_count();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return parent().is_alive(objects)
			&& parent().child_count(objects) > Group::MAX_CHILD_COUNT_TOTAL;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const->std::size_t override;

	auto child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->std::u8string override;

	auto total_count() const -> std::size_t {
		return total_count_;
	}
};

// -----------------------------------------------
// モジュールパス
// -----------------------------------------------

class HspObjectPath::Module final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t module_id_;

public:
	using HspObjectPath::new_module;
	using HspObjectPath::new_static_var;

	Module(std::shared_ptr<HspObjectPath const> parent, std::size_t module_id)
		: parent_(std::move(parent))
		, module_id_(module_id)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Module;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return module_id() == other.as_module().module_id();
	}

	auto do_hash() const -> std::size_t override {
		return module_id();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const->std::size_t override;

	auto child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->std::u8string override {
		return to_owned(objects.module_to_name(module_id()));
	}

	auto module_id() const -> std::size_t {
		return module_id_;
	}

	bool is_global(HspObjects& objects) const {
		return objects.module_global_id() == module_id();;
	}
};

// -----------------------------------------------
// 静的変数パス
// -----------------------------------------------

class HspObjectPath::StaticVar final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;
	std::size_t static_var_id_;

public:
	using HspObjectPath::new_element;
	using HspObjectPath::new_int;

	StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id)
		: parent_(std::move(parent))
		, static_var_id_(static_var_id)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::StaticVar;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return static_var_id() == other.as_static_var().static_var_id();
	}

	auto do_hash() const -> std::size_t override {
		return static_var_id();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.static_var_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return objects.static_var_path_to_child_at(*this, child_index);
	}

	auto visual_child_count(HspObjects& objects) const->std::size_t override {
		return objects.static_var_path_to_visual_child_count(*this);
	}

	auto visual_child_at(std::size_t child_index, HspObjects& objects) const->std::optional<std::shared_ptr<HspObjectPath const>> override {
		return objects.static_var_path_to_visual_child_at(*this, child_index);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return objects.static_var_path_to_name(*this);
	}

	bool is_array(HspObjects& objects) const override {
		return objects.static_var_path_is_array(*this);
	}

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto type(HspObjects& objects) const -> hsx::HspType {
		return objects.static_var_path_to_type(*this);
	}

	auto type_name(HspObjects& objects) const -> std::u8string_view {
		return objects.type_to_name(type(objects));
	}

	auto lengths(HspObjects& objects) const -> hsx::HspDimIndex {
		return metadata(objects).lengths();
	}

	auto metadata(HspObjects& objects) const -> hsx::HspVarMetadata {
		return objects.static_var_path_to_metadata(*this);
	}
};

// -----------------------------------------------
// 配列要素
// -----------------------------------------------

class HspObjectPath::Element final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	hsx::HspDimIndex indexes_;

public:
	using HspObjectPath::new_label;
	using HspObjectPath::new_str;
	using HspObjectPath::new_double;
	using HspObjectPath::new_int;
	using HspObjectPath::new_flex;
	using HspObjectPath::new_unknown;

	Element(std::shared_ptr<HspObjectPath const> parent, hsx::HspDimIndex const& indexes)
		: parent_(std::move(parent))
		, indexes_(indexes)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Element;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return indexes() == other.as_element().indexes();
	}

	auto do_hash() const -> std::size_t override {
		return indexes().hash();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return objects.element_path_is_alive(*this);
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.element_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return objects.element_path_to_child_at(*this, child_index);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return objects.element_path_to_name(*this);
	}

	auto indexes() const -> hsx::HspDimIndex const& {
		return indexes_;
	}
};

// -----------------------------------------------
// 引数
// -----------------------------------------------

class HspObjectPath::Param final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	hsx::HspParamType param_type_;
	std::size_t param_index_;

public:
	using HspObjectPath::new_element;
	using HspObjectPath::new_label;
	using HspObjectPath::new_str;
	using HspObjectPath::new_double;
	using HspObjectPath::new_int;

	Param(std::shared_ptr<HspObjectPath const> parent, hsx::HspParamType param_type, std::size_t param_index)
		: parent_(std::move(parent))
		, param_type_(param_type)
		, param_index_(param_index)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Param;
	}

	bool does_equal(HspObjectPath const& other) const override {
		auto const& o = other.as_param();
		return param_type() == o.param_type() && param_index() == o.param_index();
	}

	auto do_hash() const -> std::size_t override {
		return param_index();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.param_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return objects.param_path_to_child_at(*this, child_index);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return objects.param_path_to_name(*this);
	}

	auto param_type() const -> hsx::HspParamType {
		return param_type_;
	}

	auto param_index() const -> std::size_t {
		return param_index_;
	}

	auto var_metadata(HspObjects& objects) const->std::optional<hsx::HspVarMetadata> {
		return objects.param_path_to_var_metadata(*this);
	}
};

// -----------------------------------------------
// ラベル
// -----------------------------------------------

class HspObjectPath::Label final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Label(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Label;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	// NOTE: ラベル名ではなくパス自体の名前
	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"label");
	}

	bool is_null(HspObjects& objects) const {
		return objects.label_path_is_null(*this);
	}

	auto static_label_name(HspObjects& objects) const -> std::optional<std::u8string> {
		return objects.label_path_to_static_label_name(*this);
	}

	auto static_label_id(HspObjects& objects) const -> std::optional<std::size_t> {
		return objects.label_path_to_static_label_id(*this);
	}
};

// -----------------------------------------------
// 文字列
// -----------------------------------------------

class HspObjectPath::Str final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Str(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Str;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"str");
	}

	auto value(HspObjects& objects) const -> HsxStrSpan {
		return objects.str_path_to_value(*this);
	}
};

// -----------------------------------------------
// 浮動小数点数
// -----------------------------------------------

class HspObjectPath::Double final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Double(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Double;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"double");
	}

	auto value(HspObjects& objects) const -> HsxDouble {
		return objects.double_path_to_value(*this);
	}
};

// -----------------------------------------------
// 整数
// -----------------------------------------------

class HspObjectPath::Int final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Int(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Int;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"int");
	}

	auto value(HspObjects& objects) const -> HsxInt {
		return objects.int_path_to_value(*this);
	}
};

// -----------------------------------------------
// フレックス
// -----------------------------------------------

class HspObjectPath::Flex final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	using HspObjectPath::new_param;

	explicit Flex(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Flex;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.flex_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return objects.flex_path_to_child_at(*this, child_index);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		// NOTE: HSP 的には struct や flex ではなく「モジュール変数」なため
		return to_owned(u8"module");
	}

	auto is_nullmod(HspObjects& objects) const -> std::optional<bool> {
		return objects.flex_path_is_nullmod(*this);
	}

	auto is_clone(HspObjects& objects) const->std::optional<bool> {
		return objects.flex_path_is_clone(*this);
	}

	auto module_name(HspObjects& objects) const -> std::u8string {
		return objects.flex_path_to_module_name(*this);
	}
};

// -----------------------------------------------
// アンノウン
// -----------------------------------------------

class HspObjectPath::Unknown final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Unknown(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Unknown;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"unknown");
	}
};

// -----------------------------------------------
// システム変数リスト
// -----------------------------------------------

class HspObjectPath::SystemVarList final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit SystemVarList(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::SystemVarList;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const->std::size_t override;

	auto child_at(std::size_t child_index, HspObjects& objects) const->std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"システム変数");
	}
};

// -----------------------------------------------
// システム変数
// -----------------------------------------------

class HspObjectPath::SystemVar final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	hsx::HspSystemVarKind system_var_kind_;

public:
	using HspObjectPath::new_str;
	using HspObjectPath::new_double;
	using HspObjectPath::new_int;
	using HspObjectPath::new_flex;

	SystemVar(std::shared_ptr<HspObjectPath const> parent, hsx::HspSystemVarKind system_var_kind)
		: parent_(std::move(parent))
		, system_var_kind_(system_var_kind)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::SystemVar;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return system_var_kind() == other.as_system_var().system_var_kind();
	}

	auto do_hash() const -> std::size_t override {
		return (std::size_t)system_var_kind();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.system_var_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return objects.system_var_path_to_child_at(*this, child_index);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return objects.system_var_path_to_name(*this);
	}

	auto system_var_kind() const -> hsx::HspSystemVarKind {
		return system_var_kind_;
	}
};

// -----------------------------------------------
// コールスタック
// -----------------------------------------------

class HspObjectPath::CallStack final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	using HspObjectPath::new_call_frame;

	explicit CallStack(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::CallStack;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.call_stack_path_to_call_frame_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(child_index < child_count(objects));

		// 逆順
		auto index = child_count(objects) - 1 - child_index;

		auto key_opt = objects.call_stack_path_to_call_frame_key_at(*this, index);
		if (!key_opt) {
			assert(false && u8"コールフレームを取得できません");
			return new_unavailable(to_owned(u8"コールフレームを取得できません"));
		}

		return new_call_frame(*key_opt);
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"呼び出し");
	}
};

// -----------------------------------------------
// コールフレーム
// -----------------------------------------------

class HspObjectPath::CallFrame final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	WcCallFrameKey key_;

public:
	using HspObjectPath::new_param;

	CallFrame(std::shared_ptr<HspObjectPath const> parent, WcCallFrameKey const& key)
		: parent_(std::move(parent))
		, key_(key)
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::CallFrame;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return key() == other.as_call_frame().key();
	}

	auto do_hash() const -> std::size_t override {
		return key_.call_frame_id();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return objects.call_frame_path_is_alive(*this);
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return objects.call_frame_path_to_child_count(*this);
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		auto child_opt = objects.call_frame_path_to_child_at(*this, child_index);
		if (!child_opt) {
			return new_unavailable(to_owned(u8"エラーが発生するおそれがあるため、この引数は表示されません"));
		}

		return *child_opt;
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		auto name_opt = objects.call_frame_path_to_name(*this);
		if (!name_opt) {
			return to_owned(u8"???");
	}
		return *std::move(name_opt);
	}

	auto key() const -> WcCallFrameKey {
		return key_;
	}

	auto signature(HspObjects& objects) const->std::optional<std::vector<std::u8string_view>> {
		return objects.call_frame_path_to_signature(*this);
	}

	auto full_path(HspObjects& objects) const -> std::optional<std::u8string_view> {
		return objects.call_frame_path_to_full_path(*this);
	}

	auto line_index(HspObjects& objects) const -> std::optional<std::size_t> {
		return objects.call_frame_path_to_line_index(*this);
	}
};

// -----------------------------------------------
// 全般
// -----------------------------------------------

class HspObjectPath::General final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit General(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::General;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"全般");
	}

	auto content(HspObjects& objects) const -> std::u8string {
		return objects.general_to_content();
	}
};

// -----------------------------------------------
// ログ
// -----------------------------------------------

class HspObjectPath::Log final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Log(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Log;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"ログ");
	}

	auto content(HspObjects& objects) const -> std::u8string_view {
		return objects.log_to_content();
	}

	void append(std::u8string_view text, HspObjects& objects) const {
		objects.log_do_append(text);
	}

	void clear(HspObjects& objects) const {
		objects.log_do_clear();
	}
};

// -----------------------------------------------
// スクリプト
// -----------------------------------------------

class HspObjectPath::Script final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	explicit Script(std::shared_ptr<HspObjectPath const> parent)
		: parent_(std::move(parent))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Script;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"スクリプト");
	}

	auto content(HspObjects& objects) const -> std::u8string_view {
		return objects.script_to_content();
	}

	// :thinking_face:
	auto current_line(HspObjects& objects) const -> std::size_t {
		return objects.script_to_current_line();
	}
};

// -----------------------------------------------
// 利用不能
// -----------------------------------------------

class HspObjectPath::Unavailable final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	std::u8string reason_;

public:
	Unavailable(std::shared_ptr<HspObjectPath const> parent, std::u8string&& reason)
		: parent_(std::move(parent))
		, reason_(std::move(reason))
	{
	}

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Unavailable;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto do_hash() const -> std::size_t override {
		return 0;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t child_index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return self();
	}

	auto name(HspObjects& objects) const -> std::u8string override {
		return to_owned(u8"(利用不能)");
	}

	auto reason() const->std::u8string_view {
		return reason_;
	}
};

// -----------------------------------------------
// ビジター
// -----------------------------------------------

class HspObjectPath::Visitor {
	HspObjects& objects_;

public:
	explicit Visitor(HspObjects& objects)
		: objects_(objects)
	{
	}

	virtual void accept(HspObjectPath const& path) {
		switch (path.kind()) {
		case HspObjectKind::Root:
			on_root(path.as_root());
			return;

		case HspObjectKind::Group:
			on_group(path.as_group());
			return;

		case HspObjectKind::Ellipsis:
			on_ellipsis(path.as_ellipsis());
			return;

		case HspObjectKind::Module:
			on_module(path.as_module());
			return;

		case HspObjectKind::StaticVar:
			on_static_var(path.as_static_var());
			return;

		case HspObjectKind::Element:
			on_element(path.as_element());
			return;

		case HspObjectKind::Param:
			on_param(path.as_param());
			return;

		case HspObjectKind::Label:
			on_label(path.as_label());
			return;

		case HspObjectKind::Str:
			on_str(path.as_str());
			return;

		case HspObjectKind::Double:
			on_double(path.as_double());
			return;

		case HspObjectKind::Int:
			on_int(path.as_int());
			return;

		case HspObjectKind::Flex:
			on_flex(path.as_flex());
			return;

		case HspObjectKind::Unknown:
			on_unknown(path.as_unknown());
			return;

		case HspObjectKind::SystemVarList:
			on_system_var_list(path.as_system_var_list());
			return;

		case HspObjectKind::SystemVar:
			on_system_var(path.as_system_var());
			return;

		case HspObjectKind::CallStack:
			on_call_stack(path.as_call_stack());
			return;

		case HspObjectKind::CallFrame:
			on_call_frame(path.as_call_frame());
			return;

		case HspObjectKind::General:
			on_general(path.as_general());
			return;

		case HspObjectKind::Log:
			on_log(path.as_log());
			return;

		case HspObjectKind::Script:
			on_script(path.as_script());
			return;

		case HspObjectKind::Unavailable:
			on_unavailable(path.as_unavailable());
			return;

		default:
			assert(false && u8"Unknown HspObjectKind");
			throw new std::exception{};
		}
	}

	virtual void accept_default(HspObjectPath const& path) {
		accept_children(path);
	}

	virtual void accept_parent(HspObjectPath const& path) {
		if (path.kind() == HspObjectKind::Root) {
			return;
		}

		accept(path.parent());
	}

	virtual void accept_children(HspObjectPath const& path) {
		auto child_count = path.child_count(objects());
		for (auto i = std::size_t{}; i < child_count; i++) {
			accept(*path.child_at(i, objects()));
		}
	}

	virtual void on_root(HspObjectPath::Root const& path) {
		accept_default(path);
	}

	virtual void on_group(HspObjectPath::Group const& path) {
		accept_default(path);
	}

	virtual void on_ellipsis(HspObjectPath::Ellipsis const& path) {
		accept_default(path);
	}

	virtual void on_module(HspObjectPath::Module const& path) {
		accept_default(path);
	}

	virtual void on_static_var(HspObjectPath::StaticVar const& path) {
		accept_default(path);
	}

	virtual void on_element(HspObjectPath::Element const& path) {
		accept_default(path);
	}

	virtual void on_param(HspObjectPath::Param const& path) {
		accept_default(path);
	}

	virtual void on_label(HspObjectPath::Label const& path) {
		accept_default(path);
	}

	virtual void on_str(HspObjectPath::Str const& path) {
		accept_default(path);
	}

	virtual void on_double(HspObjectPath::Double const& path) {
		accept_default(path);
	}

	virtual void on_int(HspObjectPath::Int const& path) {
		accept_default(path);
	}

	virtual void on_flex(HspObjectPath::Flex const& path) {
		accept_default(path);
	}

	virtual void on_unknown(HspObjectPath::Unknown const& path) {
		accept_default(path);
	}

	virtual void on_system_var_list(HspObjectPath::SystemVarList const& path) {
		accept_default(path);
	}

	virtual void on_system_var(HspObjectPath::SystemVar const& path) {
		accept_default(path);
	}

	virtual void on_call_stack(HspObjectPath::CallStack const& path) {
		accept_default(path);
	}

	virtual void on_call_frame(HspObjectPath::CallFrame const& path) {
		accept_default(path);
	}

	virtual void on_general(HspObjectPath::General const& path) {
		accept_default(path);
	}

	virtual void on_log(HspObjectPath::Log const& path) {
		accept_default(path);
	}

	virtual void on_script(HspObjectPath::Script const& path) {
		accept_default(path);
	}

	virtual void on_unavailable(HspObjectPath::Unavailable const& path) {
		accept_default(path);
	}

protected:
	auto objects() const -> HspObjects& {
		return objects_;
	}
};
