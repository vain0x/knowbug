
#pragma once

#include "hpiutil_fwd.hpp"

namespace hpiutil {

// ソースコード上の位置を表す
class SourcePos
{
public:
	SourcePos()
		: SourcePos(nullptr, 0)
	{}

	SourcePos(char const* fileRefName, int line)
		: fileRefName_(fileRefName ? fileRefName : "???")
		, line_(line >= 0 ? line : 0)
	{}

	SourcePos(SourcePos const&) = default;
	auto operator=(SourcePos const&) -> SourcePos& = default;

	auto fileRefName() const -> char const*
	{
		return fileRefName_;
	}

	auto line() const -> int
	{
		return line_;
	}

	auto toString() const -> std::string
	{
		return "#" + std::to_string(1 + line())
			+ " " + fileRefName();
	}
private:
	char const* fileRefName_;

	// 0-indexed
	int line_;
};

} //namespace hpiutil
