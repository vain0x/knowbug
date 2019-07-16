
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

	auto toTuple() const -> std::tuple<std::string, int>
	{
		return std::make_tuple(fileRefName(), line());
	}
	bool operator==(SourcePos const& rhs) const { return toTuple() == rhs.toTuple(); }
	bool operator!=(SourcePos const& rhs) const { return std::rel_ops::operator!=(*this, rhs); }

private:
	char const* fileRefName_;

	// 0-indexed
	int line_;
};

} //namespace hpiutil

namespace std {

template<>
struct hash<hpiutil::SourcePos>
{
	auto operator()(hpiutil::SourcePos const& src) const -> size_t
	{
		return hash<decltype(src.fileRefName())> {}(src.fileRefName())
			^ hash<decltype(src.line())> {}(src.line());
	}
};

} //namespace std
