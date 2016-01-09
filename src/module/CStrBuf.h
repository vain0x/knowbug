
#pragma once
#include <string>

class CStrBuf
{
	using string = std::string;
public:
	CStrBuf();

	auto limit() const -> size_t { return lenLimit_; }
	void limit(size_t newLimit);

	void append(char const* s);
	void append(char const* s, size_t len);

	auto get() -> string const& { return buf_; }
	auto getMove() -> string&& { return std::move(buf_); }
private:
	string buf_;
	size_t lenLimit_;
};
