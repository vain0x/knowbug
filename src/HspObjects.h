#pragma once

#include <memory>
#include <string>
#include "encoding.h"
#include "HspTypes.h"
#include "HspObjectPath.h"

class HspDebugApi;
class HspStaticVars;

// HSP のオブジェクト (モジュール、変数、値など) に関して
// knowbug が知りたい情報を最適なインターフェイスで提供する。
class HspObjects {
public:
	class Module;
	class TypeData;

private:
	HspDebugApi& api_;
	HspStaticVars& static_vars_;

	std::vector<Module> modules_;
	std::vector<TypeData> types_;

public:
	HspObjects(HspDebugApi& api, HspStaticVars& static_vars);

	auto type_name(HspType type) const->HspStringView;

	auto module_global_id() const->std::size_t;

	auto module_count() const->std::size_t;

	auto module_name(std::size_t module_id) const->HspStringView;

	auto module_var_count(std::size_t module_id) const->std::size_t;

	auto module_var_at(std::size_t module_id, std::size_t index) const->std::size_t;

	auto static_var_name(std::size_t static_var_id)->std::string;

	bool static_var_is_array(std::size_t static_var_id);

	// FIXME: 一時的に存在する。knowbug は PVal* を直接触るべきでない
	auto static_var_to_pval(std::size_t static_var_id)->PVal*;

	auto static_var_to_type(std::size_t static_var_id)->HspType;

	auto static_var_child_count(HspObjectPath::StaticVar const& path) const->std::size_t;

	auto static_var_child_at(HspObjectPath::StaticVar const& path, std::size_t child_index) const->std::shared_ptr<HspObjectPath const>;

	auto static_var_metadata(HspObjectPath::StaticVar const& path) -> HspVarMetadata;

	auto str_path_to_value(HspObjectPath::Str const& path) const->HspStr;

	auto path_to_int(HspObjectPath::Int const& path) const->HspInt;

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
