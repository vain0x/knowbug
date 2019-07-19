#pragma once

#include <vector>
#include "encoding.h"
#include "test_suite.h"

class ModuleTreeListener {
public:
	virtual ~ModuleTreeListener() {
	}

	virtual void begin_module(Utf8StringView const& module_name) = 0;

	virtual void end_module() = 0;

	virtual void add_var(std::size_t var_id, Utf8StringView const& var_name) = 0;
};

extern void traverse_module_tree(std::vector<Utf8String> const& var_names, ModuleTreeListener& listener);

extern void module_tree_tests(Tests& tests);
