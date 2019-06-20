#pragma once

#include <memory>
#include <string>
#include "encoding.h"
#include "HspDebugApi.h"

class HspStaticVars;

// HSP のメモリ空間上に存在するオブジェクトに関する情報を取得・変更するためのインターフェイス
class HspObjects {
public:
	class Module;

private:
	HspDebugApi& api_;
	HspStaticVars& static_vars_;

	std::vector<Module> modules_;

public:
	HspObjects(HspDebugApi& api, HspStaticVars& static_vars);

	auto module_global_id() const->std::size_t;

	auto module_count() const->std::size_t;

	auto module_name(std::size_t module_id) const->HspStringView;

	auto module_var_count(std::size_t module_id) const->std::size_t;

	auto module_var_at(std::size_t module_id, std::size_t index) const->std::size_t;

	auto static_var_name(std::size_t static_var_id)->std::string;

	bool static_var_is_array(std::size_t static_var_id);

	auto static_var_to_pval(std::size_t static_var_id)->PVal*;

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
