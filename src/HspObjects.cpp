#include "hpiutil/hpiutil.hpp"
#include "HspDebugApi.h"
#include "HspObjects.h"
#include "HspStaticVars.h"

auto const GLOBAL_MODULE_ID = std::size_t{ 0 };

static auto var_name_to_scope_resolution(char const* var_name) -> char const* {
	return std::strchr(var_name, '@');
}

// 変数をモジュールごとに分類する。
static auto group_vars_by_module(std::vector<HspString> const& var_names) -> std::vector<HspObjects::Module> {
	auto modules = std::vector<HspObjects::Module>{};

	// モジュール名、変数名、変数IDの組
	auto tuples = std::vector<std::tuple<HspStringView, HspStringView, std::size_t>>{};

	{
		for (auto vi = std::size_t{}; vi < var_names.size(); vi++) {
			auto&& var_name = var_names[vi];

			auto p = var_name_to_scope_resolution(var_name.data());
			auto module_name = HspStringView{ (p ? p : "@") };

			tuples.emplace_back(module_name, var_name.as_ref(), vi);
		}
	}

	std::sort(std::begin(tuples), std::end(tuples));

	// モジュールと変数の関係を構築する。
	{
		modules.emplace_back(HspStringView{ "@" }.to_owned());

		for (auto&& t : tuples) {
			auto module_name_ref = std::get<0>(t);
			auto var_name_ref = std::get<1>(t);
			auto vi = std::get<2>(t);

			{
				auto module_id = modules.size() - 1;
				if (!(modules[module_id].name() == module_name_ref)) {
					modules.emplace_back(module_name_ref.to_owned());
				}
			}

			{
				auto module_id = modules.size() - 1;
				assert(modules[module_id].name() == module_name_ref);
				modules[module_id].add_var(vi);
			}
		}
	}

	// 事後条件
	assert(modules[GLOBAL_MODULE_ID].name() == HspStringView{ "@" });
	return modules;
}

// -----------------------------------------------
// HspObjects
// -----------------------------------------------

HspObjects::HspObjects(HspDebugApi& api, HspStaticVars& static_vars)
	: api_(api)
	, static_vars_(static_vars)
	, modules_(group_vars_by_module(static_vars.get_all_names()))
{
}

auto HspObjects::module_global_id() const->std::size_t {
	return GLOBAL_MODULE_ID;
}

auto HspObjects::module_count() const->std::size_t {
	return modules_.size();
}

auto HspObjects::module_name(std::size_t module_id) const -> HspStringView {
	return modules_.at(module_id).name();
}

auto HspObjects::module_var_count(std::size_t module_id) const->std::size_t {
	return modules_.at(module_id).var_ids().size();
}

auto HspObjects::module_var_at(std::size_t module_id, std::size_t index) const->std::size_t {
	return modules_.at(module_id).var_ids().at(index);
}

auto HspObjects::static_var_name(std::size_t static_var_id)->std::string {
	return *api_.static_var_find_name(static_var_id);
}

bool HspObjects::static_var_is_array(std::size_t static_var_id) {
	return hpiutil::PVal_isStandardArray(&hpiutil::staticVars()[static_var_id]);
}

auto HspObjects::static_var_to_pval(std::size_t static_var_id)->PVal* {
	return (PVal*)&hpiutil::staticVars()[static_var_id];
}

// -----------------------------------------------
// HspObjects::Module
// -----------------------------------------------

HspObjects::Module::Module(HspString&& name)
	: name_(std::move(name))
	, var_ids_()
{
}

void HspObjects::Module::add_var(std::size_t static_var_id) {
	var_ids_.emplace_back(static_var_id);
}
