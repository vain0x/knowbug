#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "encoding.h"
#include "hsx.h"
#include "hsp_object_path_fwd.h"
#include "hsp_wrap_call.h"

class SourceFileId;
class SourceFileRepository;
class SourceFileResolver;

// FIXME: インターフェイスを抽出する

// HSP のオブジェクト (モジュール、変数、値など) に関して
// knowbug が知りたい情報を最適なインターフェイスで提供する。
class HspObjects {
public:
	class CallFrame;
	class Module;
	class TypeData;

private:
	HSP3DEBUG* debug_;

	std::unique_ptr<SourceFileRepository> source_file_repository_;

	std::shared_ptr<HspObjectPath const> root_path_;

	std::vector<std::u8string> var_names_;
	std::vector<Module> modules_;
	std::vector<TypeData> types_;
	std::unordered_map<HsxLabel, std::u8string> label_names_;
	std::unordered_map<STRUCTPRM const*, std::u8string> param_names_;

	std::shared_ptr<WcDebugger> wc_debugger_;

	std::u8string log_;

public:
	HspObjects(HSP3DEBUG* debug, std::vector<std::u8string>&& var_names, std::vector<Module>&& modules, std::unordered_map<HsxLabel, std::u8string>&& label_names, std::unordered_map<STRUCTPRM const*, std::u8string>&& param_names, std::unique_ptr<SourceFileRepository>&& source_file_repository, std::shared_ptr<WcDebugger> wc_debugger);

	void initialize();

	auto root_path() const->HspObjectPath::Root const&;

	auto path_to_visual_child_count(HspObjectPath const& path)->std::size_t;

	auto path_to_visual_child_at(HspObjectPath const& path, std::size_t child_index)->std::optional<std::shared_ptr<HspObjectPath const>>;

	auto path_to_memory_view(HspObjectPath const& path) const->std::optional<MemoryView>;

	auto type_to_name(HsxVartype type) const->std::u8string_view;

	auto module_global_id() const->std::size_t;

	auto module_count() const->std::size_t;

	auto module_to_name(std::size_t module_id) const->std::u8string_view;

	auto module_to_var_count(std::size_t module_id) const->std::size_t;

	auto module_to_var_at(std::size_t module_id, std::size_t index) const->std::size_t;

	auto static_var_path_to_name(HspObjectPath::StaticVar const& path)->std::u8string;

	bool static_var_path_is_array(HspObjectPath::StaticVar const& path);

	auto static_var_path_to_type(HspObjectPath::StaticVar const& path)->HsxVartype;

	auto static_var_path_to_child_count(HspObjectPath::StaticVar const& path) const->std::size_t;

	auto static_var_path_to_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index) const->std::shared_ptr<HspObjectPath const>;

	auto static_var_path_to_visual_child_count(HspObjectPath::StaticVar const& path)->std::size_t;

	auto static_var_path_to_visual_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index)->std::optional<std::shared_ptr<HspObjectPath const>>;

	auto static_var_path_to_metadata(HspObjectPath::StaticVar const& path) -> std::optional<HsxVarMetadata>;

	auto element_path_is_alive(HspObjectPath::Element const& path) const->bool;

	auto element_path_to_child_count(HspObjectPath::Element const& path) const -> std::size_t;

	auto element_path_to_child_at(HspObjectPath::Element const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const>;

	auto element_path_to_name(HspObjectPath::Element const& path) const->std::u8string;

	auto param_path_to_child_count(HspObjectPath::Param const& path) const -> std::size_t;

	auto param_path_to_child_at(HspObjectPath::Param const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const>;

	auto param_path_to_name(HspObjectPath::Param const& path) const -> std::u8string;

	auto param_path_to_var_metadata(HspObjectPath::Param const& path) const->std::optional<HsxVarMetadata>;

	bool label_path_is_null(HspObjectPath::Label const& path) const;

	auto label_path_to_static_label_name(HspObjectPath::Label const& path) const -> std::optional<std::u8string>;

	auto label_path_to_static_label_id(HspObjectPath::Label const& path) const -> std::optional<std::size_t>;

	auto str_path_to_value(HspObjectPath::Str const& path) const->HsxStrSpan;

	auto double_path_to_value(HspObjectPath::Double const& path) const->HsxDouble;

	auto int_path_to_value(HspObjectPath::Int const& path) const->HsxInt;

	auto flex_path_to_child_count(HspObjectPath::Flex const& path)->std::size_t;

	auto flex_path_to_child_at(HspObjectPath::Flex const& path, std::size_t index)->std::shared_ptr<HspObjectPath const>;

	auto flex_path_is_nullmod(HspObjectPath::Flex const& path)->std::optional<bool>;

	auto flex_path_is_clone(HspObjectPath::Flex const& path)->std::optional<bool>;

	auto flex_path_to_module_name(HspObjectPath::Flex const& path) -> std::u8string;

	auto system_var_path_to_child_count(HspObjectPath::SystemVar const& path) const -> std::size_t;

	auto system_var_path_to_child_at(HspObjectPath::SystemVar const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const>;

	auto system_var_path_to_name(HspObjectPath::SystemVar const& path) const -> std::u8string;

	auto call_stack_path_to_call_frame_count(HspObjectPath::CallStack const& path) const -> std::size_t;

	auto call_stack_path_to_call_frame_key_at(HspObjectPath::CallStack const& path, std::size_t call_frame_index) const -> std::optional<WcCallFrameKey>;

	auto call_frame_path_to_name(HspObjectPath::CallFrame const& path) const -> std::optional<std::u8string>;

	auto call_frame_path_is_alive(HspObjectPath::CallFrame const& path) const -> bool;

	auto call_frame_path_to_child_count(HspObjectPath::CallFrame const& path) const -> std::size_t;

	auto call_frame_path_to_child_at(HspObjectPath::CallFrame const& path, std::size_t child_index) const -> std::optional<std::shared_ptr<HspObjectPath const>>;

	auto call_frame_path_to_signature(HspObjectPath::CallFrame const& path) const->std::optional<std::vector<std::u8string_view>>;

	auto call_frame_path_to_full_path(HspObjectPath::CallFrame const& path) const -> std::optional<std::u8string_view>;

	auto call_frame_path_to_line_index(HspObjectPath::CallFrame const& path) const -> std::optional<std::size_t>;

	auto general_to_content() -> std::u8string;

	auto log_to_content() const -> std::u8string_view;

	// ログに追記する。末尾の改行文字は追加されない。
	void log_do_append(std::u8string_view text);

	void log_do_clear();

	auto script_to_full_path() const -> std::optional<OsStringView>;

	auto script_to_content() const -> std::u8string_view;

	auto script_to_current_file() const->std::optional<std::size_t>;

	auto script_to_current_line() const -> std::size_t;

	auto script_to_current_location_summary() const->std::u8string;

	// :thinking_face:
	void script_do_update_location();

	auto source_file_to_full_path(std::size_t source_file_id) const->std::optional<std::u8string_view>;

	auto source_file_to_content(std::size_t source_file_id) const->std::optional<std::u8string_view>;

private:
	auto debug() -> HSP3DEBUG* {
		return debug_;
	}

	auto debug() const -> HSP3DEBUG const* {
		return debug_;
	}

	auto context() const->HSPCTX const*;

public:
	class Module {
		std::u8string name_;

		// モジュールに含まれる静的変数のIDのリスト。変数名について昇順。
		std::vector<std::size_t> var_ids_;

	public:
		Module(std::u8string&& name);

		auto name() const->std::u8string_view {
			return name_;
		}

		auto var_ids() const->std::vector<std::size_t> const& {
			return var_ids_;
		}

		void add_var(std::size_t static_var_id);
	};
};

class HspObjects::TypeData {
	std::u8string name_;

public:
	explicit TypeData(std::u8string&& name);

	auto name() const -> std::u8string_view {
		return name_;
	}
};

class HspObjectsBuilder {
	std::vector<std::u8string> var_names_;

	std::unordered_map<HsxLabel, std::u8string> label_names_;

	std::unordered_map<STRUCTPRM const*, std::u8string> param_names_;

public:
	void add_var_name(char const* var_name);

	void add_label_name(int ot_index, char const* label_name, HSPCTX const* ctx);

	void add_param_name(int param_index, char const* param_name, HSPCTX const* ctx);

	void read_debug_segment(SourceFileResolver& resolver, HSPCTX const* ctx);

	auto finish(HSP3DEBUG* debug, std::unique_ptr<SourceFileRepository>&& source_file_repository)->HspObjects;
};

// 迷子

extern auto indexes_to_string(HsxIndexes indexes)->std::u8string;

extern auto var_name_to_bare_ident(std::u8string_view name)->std::u8string_view;
