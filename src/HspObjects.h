#pragma once

#include <memory>
#include <string>
#include "encoding.h"
#include "HspTypes.h"
#include "HspObjectPath.h"

namespace hpiutil {
	class DInfo;
}

class HspDebugApi;
class HspStaticVars;

class HspLogger {
public:
	virtual ~HspLogger() {
	}

	virtual auto content() const -> std::string const& = 0;
	virtual void append(char const* text) = 0;
	virtual void clear() = 0;
};

class HspScripts {
public:
	virtual ~HspScripts() {
	}

	virtual auto content(char const* file_name_ref) -> std::string const& = 0;
};

// HSP のオブジェクト (モジュール、変数、値など) に関して
// knowbug が知りたい情報を最適なインターフェイスで提供する。
class HspObjects {
public:
	class Module;
	class TypeData;

private:
	HspDebugApi& api_;
	HspLogger& logger_;
	HspScripts& scripts_;
	HspStaticVars& static_vars_;
	hpiutil::DInfo const& debug_segment_;

	std::shared_ptr<HspObjectPath const> root_path_;

	std::vector<Module> modules_;
	std::vector<TypeData> types_;

public:
	HspObjects(HspDebugApi& api, HspLogger& logger, HspScripts& scripts, HspStaticVars& static_vars, hpiutil::DInfo const& debug_segment);

	auto root_path() const->HspObjectPath::Root const&;

	auto type_to_name(HspType type) const->HspStringView;

	auto module_global_id() const->std::size_t;

	auto module_count() const->std::size_t;

	auto module_to_name(std::size_t module_id) const->HspStringView;

	auto module_to_var_count(std::size_t module_id) const->std::size_t;

	auto module_to_var_at(std::size_t module_id, std::size_t index) const->std::size_t;

	auto static_var_path_to_name(HspObjectPath::StaticVar const& path)->std::string;

	bool static_var_path_is_array(HspObjectPath::StaticVar const& path);

	// FIXME: 一時的に存在する。knowbug は PVal* を直接触るべきでない
	auto static_var_path_to_pval(HspObjectPath::StaticVar const& path)->PVal*;

	auto static_var_path_to_type(HspObjectPath::StaticVar const& path)->HspType;

	auto static_var_path_to_child_count(HspObjectPath::StaticVar const& path) const->std::size_t;

	auto static_var_path_to_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index) const->std::shared_ptr<HspObjectPath const>;

	auto static_var_path_to_metadata(HspObjectPath::StaticVar const& path) -> HspVarMetadata;

	auto element_path_to_child_count(HspObjectPath::Element const& path) const -> std::size_t;

	auto element_path_to_child_at(HspObjectPath::Element const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const>;

	auto param_path_to_child_count(HspObjectPath::Param const& path) const -> std::size_t;

	auto param_path_to_child_at(HspObjectPath::Param const& path, std::size_t child_index) const -> std::shared_ptr<HspObjectPath const>;

	auto param_path_to_name(HspObjectPath::Param const& path) const -> std::string;

	bool label_path_is_null(HspObjectPath::Label const& path) const;

	auto label_path_to_static_label_name(HspObjectPath::Label const& path) const -> std::optional<std::string>;

	auto label_path_to_static_label_id(HspObjectPath::Label const& path) const -> std::optional<std::size_t>;

	auto str_path_to_value(HspObjectPath::Str const& path) const->HspStr;

	auto double_path_to_value(HspObjectPath::Double const& path) const->HspDouble;

	auto int_path_to_value(HspObjectPath::Int const& path) const->HspInt;

	auto flex_path_to_child_count(HspObjectPath::Flex const& path)->std::size_t;

	auto flex_path_to_child_at(HspObjectPath::Flex const& path, std::size_t index)->std::shared_ptr<HspObjectPath const>;

	bool flex_path_is_nullmod(HspObjectPath::Flex const& path);

	auto flex_path_to_module_name(HspObjectPath::Flex const& path) -> char const*;

	auto log_to_content() const -> std::string const&;

	void log_do_append(char const* text);

	void log_do_clear();

	auto script_to_content() const -> std::string const&;

	auto script_to_current_line() const -> std::size_t;

public:
	class Module {
		HspString name_;

		// モジュールに含まれる静的変数のIDのリスト。変数名について昇順。
		std::vector<std::size_t> var_ids_;

	public:
		Module(HspString&& name);

		auto name() const->HspStringView {
			return name_.as_ref();
		}

		auto var_ids() const->std::vector<std::size_t> const& {
			return var_ids_;
		}

		void add_var(std::size_t static_var_id);
	};
};

class HspObjects::TypeData {
	HspString name_;

public:
	explicit TypeData(HspString&& name);

	auto name() const -> HspStringView {
		return name_.as_ref();
	}
};
