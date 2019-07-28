
#pragma once

#include <unordered_map>
#include <unordered_set>
#include "../hpiutil/hpiutil_fwd.hpp"

namespace hpiutil {

class DInfo
{
	DInfo()
	{
		parse();
	}

public:
	static auto instance() -> DInfo&;

private:
	using ident_table_t = std::unordered_map<int, char const*>;

	std::unordered_set<std::string> fileRefNames_;
	ident_table_t labelNames_;
	ident_table_t paramNames_;

public:
	auto tryFindIdent(ident_table_t const& table, int iparam) const -> char const*
	{
		auto const iter = table.find(iparam);
		return (iter != table.end()) ? iter->second : nullptr;
	}

	auto tryFindLabelName(int otIndex) const -> char const*
	{
		return tryFindIdent(labelNames_, otIndex);
	}

	auto tryFindParamName(int stprmIndex) const -> char const*
	{
		return tryFindIdent(paramNames_, stprmIndex);
	}

	auto fileRefNames() const -> decltype(fileRefNames_) const&
	{
		return fileRefNames_;
	}

private:
	void parse();
};

} // namespace hpiutil
