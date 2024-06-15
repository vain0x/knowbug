#include "pch.h"
#include <memory>
#include <optional>
#include "hsp_objects_module_tree.h"
#include "string_writer.h"

static auto const GLOBAL_MODULE_NAME = as_utf8(u8"@");

static auto var_name_to_scope_resolution(std::u8string_view var_name) -> std::optional<std::u8string_view> {
	auto ptr = std::strchr(as_native(var_name).data(), '@');
	if (!ptr) {
		return std::nullopt;
	}

	return std::make_optional(as_utf8(ptr));
}

void traverse_module_tree(std::vector<std::u8string> const& var_names, ModuleTreeListener& listener) {
	// モジュール名、変数名、変数IDの組
	auto tuples = std::vector<std::tuple<std::u8string_view, std::u8string_view, std::size_t>>{};

	{
		for (auto vi = std::size_t{}; vi < var_names.size(); vi++) {
			auto const& var_name = var_names[vi];

			auto resolution_opt = var_name_to_scope_resolution(var_name);
			auto module_name = resolution_opt ? *resolution_opt : GLOBAL_MODULE_NAME;

			tuples.emplace_back(module_name, var_name, vi);
		}
	}

	std::sort(std::begin(tuples), std::end(tuples));

	// モジュールと変数の関係を構築する。
	{
		auto current_module_name = GLOBAL_MODULE_NAME;
		listener.begin_module(current_module_name);

		for (auto&& t : tuples) {
			auto module_name_ref = std::get<0>(t);
			auto var_name_ref = std::get<1>(t);
			auto vi = std::get<2>(t);

			if (module_name_ref != current_module_name) {
				listener.end_module();
				current_module_name = module_name_ref;
				listener.begin_module(current_module_name);
			}

			assert(current_module_name == module_name_ref);
			listener.add_var(vi, var_name_ref);
		}

		listener.end_module();
	}
}

class ModuleTreeWriter
	: public ModuleTreeListener
{
	StringWriter& writer_;

public:
	ModuleTreeWriter(StringWriter& writer)
		: writer_(writer)
	{
	}

	void begin_module(std::u8string_view module_name) override {
		writer_.cat_line(module_name);
		writer_.indent();
	}

	void end_module() override {
		writer_.unindent();
	}

	void add_var(std::size_t var_id, std::u8string_view var_name) override {
		writer_.cat(var_name);
		writer_.cat(u8" #");
		writer_.cat_size(var_id);
		writer_.cat_crlf();
	}
};

void module_tree_tests(Tests& tests) {
	auto& suite = tests.suite(u8"hsp_objects_module_tree");

	suite.test(
		u8"スコープ解決の部分を取得できる",
		[&](TestCaseContext& t) {
			if (!t.eq(*var_name_to_scope_resolution(u8"foo@bar"), u8"@bar")) {
				return false;
			}

			if (!t.eq(*var_name_to_scope_resolution(u8"foo@"), u8"@")) {
				return false;
			}

			if (!t.eq(var_name_to_scope_resolution(u8"foo") == std::nullopt, true)) {
				return false;
			}

			return true;
		});

	suite.test(
		u8"モジュールツリーを構築できる",
		[&](TestCaseContext& t) {
			auto w = StringWriter{};
			auto listener = ModuleTreeWriter{ w };
			auto var_names = std::vector<std::u8string>{
				to_owned(u8"s1@m1"),
				to_owned(u8"t2@m2"),
				to_owned(u8"s2@m1"),
				to_owned(u8"t1@m2"),
				to_owned(u8"g1"),
				to_owned(u8"g2")
			};

			traverse_module_tree(var_names, listener);

			auto expected = as_utf8(
				u8"@\r\n"
				u8"  g1 #4\r\n"
				u8"  g2 #5\r\n"
				u8"@m1\r\n"
				u8"  s1@m1 #0\r\n"
				u8"  s2@m1 #2\r\n"
				u8"@m2\r\n"
				u8"  t1@m2 #3\r\n"
				u8"  t2@m2 #1\r\n"
			);
			return t.eq(as_view(w), expected);
		});

	suite.test(
		u8"変数がないケース",
		[&](TestCaseContext& t) {
			auto w = StringWriter{};
			auto listener = ModuleTreeWriter{ w };

			traverse_module_tree(std::vector<std::u8string>{}, listener);
			return t.eq(as_view(w), u8"@\r\n");
		});
}
