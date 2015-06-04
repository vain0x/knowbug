
#pragma once
#include <string>

class CStrBuf {
	using string = std::string;
public:
	CStrBuf();

	size_t limit() const { return lenLimit_; }
	void limit(size_t newLimit);

	void append(char const* s);
	void append(char const* s, size_t len);

	string const& get() { return buf_; }
	string&& getMove() { return std::move(buf_); }
private:
	string buf_;
	size_t lenLimit_;
};
