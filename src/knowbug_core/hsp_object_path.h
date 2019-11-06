#pragma once

#include <string>
#include <vector>
#include "encoding.h"
#include "hsx.h"
#include "hsp_object_path_fwd.h"
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

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override;

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

	auto new_global_module(HspObjects& objects) const->std::shared_ptr<HspObjectPath const>;
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

	Module(std::shared_ptr<HspObjectPath const> parent, std::size_t module_id);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Module;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return module_id() == other.as_module().module_id();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const->Utf8String override;

	auto module_id() const -> std::size_t {
		return module_id_;
	}

	bool is_global(HspObjects& objects) const;
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

	StaticVar(std::shared_ptr<HspObjectPath const> parent, std::size_t static_var_id);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::StaticVar;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return static_var_id() == other.as_static_var().static_var_id();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

	bool is_array(HspObjects& objects) const override;

	auto static_var_id() const -> std::size_t {
		return static_var_id_;
	}

	auto type(HspObjects& objects) const -> hsx::HspType;

	auto type_name(HspObjects& objects) const -> Utf8StringView;

	auto lengths(HspObjects& objects) const -> hsx::HspDimIndex;

	auto metadata(HspObjects& objects) const -> hsx::HspVarMetadata;
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

	Element(std::shared_ptr<HspObjectPath const> parent, hsx::HspDimIndex const& indexes);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Element;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return indexes() == other.as_element().indexes();
	}

	auto is_alive(HspObjects& objects) const -> bool override;

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

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

	Param(std::shared_ptr<HspObjectPath const> parent, hsx::HspParamType param_type, std::size_t param_index);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Param;
	}

	bool does_equal(HspObjectPath const& other) const override {
		auto&& o = other.as_param();
		return param_type() == o.param_type() && param_index() == o.param_index();
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

	auto param_type() const -> hsx::HspParamType {
		return param_type_;
	}

	auto param_index() const -> std::size_t {
		return param_index_;
	}

	auto var_metadata(HspObjects& objects) const->std::optional<hsx::HspVarMetadata>;
};

// -----------------------------------------------
// ラベル
// -----------------------------------------------

class HspObjectPath::Label final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Label(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Label;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	// NOTE: ラベル名ではなくパス自体の名前
	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"label"));
	}

	bool is_null(HspObjects& objects) const;

	auto static_label_name(HspObjects& objects) const -> std::optional<Utf8String>;

	auto static_label_id(HspObjects& objects) const -> std::optional<std::size_t>;
};

// -----------------------------------------------
// 文字列
// -----------------------------------------------

class HspObjectPath::Str final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Str(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Str;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"str"));
	}

	auto value(HspObjects& objects) const -> hsx::HspStr;
};

// -----------------------------------------------
// 浮動小数点数
// -----------------------------------------------

class HspObjectPath::Double final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Double(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Double;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"double"));
	}

	auto value(HspObjects& objects) const -> hsx::HspDouble;
};

// -----------------------------------------------
// 整数
// -----------------------------------------------

class HspObjectPath::Int final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Int(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Int;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"int"));
	}

	auto value(HspObjects& objects) const -> hsx::HspInt;
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

	Flex(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Flex;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override {
		// NOTE: HSP 的には struct や flex ではなく「モジュール変数」なため
		return to_owned(as_utf8(u8"module"));
	}

	auto is_nullmod(HspObjects& objects) const -> std::optional<bool>;

	auto is_clone(HspObjects& objects) const->std::optional<bool>;

	auto module_name(HspObjects& objects) const -> Utf8String;
};

// -----------------------------------------------
// アンノウン
// -----------------------------------------------

class HspObjectPath::Unknown final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Unknown(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Unknown;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw new std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"unknown"));
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
	SystemVarList(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::SystemVarList;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"システム変数"));
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

	SystemVar(std::shared_ptr<HspObjectPath const> parent, hsx::HspSystemVarKind system_var_kind);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::SystemVar;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return system_var_kind() == other.as_system_var().system_var_kind();
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

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

	CallStack(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::CallStack;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto is_alive(HspObjects& objects) const -> bool override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"呼び出し"));
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

	CallFrame(std::shared_ptr<HspObjectPath const> parent, WcCallFrameKey const& key);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::CallFrame;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return key() == other.as_call_frame().key();
	}

	auto is_alive(HspObjects& objects) const -> bool override;

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override;

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override;

	auto name(HspObjects& objects) const -> Utf8String override;

	auto key() const -> WcCallFrameKey {
		return key_;
	}

	auto signature(HspObjects& objects) const->std::optional<std::vector<Utf8StringView>>;

	auto full_path(HspObjects& objects) const -> std::optional<Utf8StringView>;

	auto line_index(HspObjects& objects) const -> std::optional<std::size_t>;
};

// -----------------------------------------------
// 全般
// -----------------------------------------------

class HspObjectPath::General final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	General(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::General;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
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

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"全般"));
	}

	auto content(HspObjects& objects) const -> Utf8String;
};

// -----------------------------------------------
// ログ
// -----------------------------------------------

class HspObjectPath::Log final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Log(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Log;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
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

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"ログ"));
	}

	auto content(HspObjects& objects) const -> Utf8StringView;

	void append(Utf8StringView const& text, HspObjects& objects) const;

	void clear(HspObjects& objects) const;
};

// -----------------------------------------------
// スクリプト
// -----------------------------------------------

class HspObjectPath::Script final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

public:
	Script(std::shared_ptr<HspObjectPath const> parent);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Script;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
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

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		assert(false && u8"no children");
		throw std::exception{};
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"スクリプト"));
	}

	auto content(HspObjects& objects) const -> Utf8StringView;

	// :thinking_face:
	auto current_line(HspObjects& objects) const -> std::size_t;
};

// -----------------------------------------------
// 利用不能
// -----------------------------------------------

class HspObjectPath::Unavailable final
	: public HspObjectPath
{
	std::shared_ptr<HspObjectPath const> parent_;

	Utf8String reason_;

public:
	Unavailable(std::shared_ptr<HspObjectPath const> parent, Utf8String&& reason);

	auto kind() const -> HspObjectKind override {
		return HspObjectKind::Unavailable;
	}

	bool does_equal(HspObjectPath const& other) const override {
		return true;
	}

	auto parent() const -> HspObjectPath const& override {
		return *parent_;
	}

	auto child_count(HspObjects& objects) const -> std::size_t override {
		return 0;
	}

	auto child_at(std::size_t index, HspObjects& objects) const -> std::shared_ptr<HspObjectPath const> override {
		return self();
	}

	auto name(HspObjects& objects) const -> Utf8String override {
		return to_owned(as_utf8(u8"(利用不能)"));
	}

	auto reason() const->Utf8StringView;
};

// -----------------------------------------------
// ビジター
// -----------------------------------------------

class HspObjectPath::Visitor {
	HspObjects& objects_;

public:
	explicit Visitor(HspObjects& objects);

	virtual void accept(HspObjectPath const& path);

	virtual void accept_default(HspObjectPath const& path);

	virtual void accept_parent(HspObjectPath const& path);

	virtual void accept_children(HspObjectPath const& path);

	virtual void on_root(HspObjectPath::Root const& path) {
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
