#ifndef IG_LINE_DELIMITED_STRING_H
#define IG_LINE_DELIMITED_STRING_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>

static auto countIndents(char const* s) -> size_t
{
	auto i = size_t { 0 };
	for ( ; s[i] == '\t' || s[i] == ' '; ++i );
	return i;
}

//行ごとに区切られた変更不可な文字列
class LineDelimitedString
{
	using string = std::string;

	string base_;

	//[i]: i行目の先頭の字下げ後への添字
	//back(): 末尾への添字
	std::vector<size_t> index_;

public:
	LineDelimitedString(std::istream& is)
	{
		auto linebuf = std::array<char, 0x1000> {};
		auto idx = size_t { 0 };
		do {
			is.getline(linebuf.data(), linebuf.size());
			index_.push_back(idx + countIndents(linebuf.data()));

			auto const len = std::strlen(linebuf.data());
			base_
				.append(linebuf.data(), len)
				.append("\r\n")
				;
			idx += len + 2;
		} while ( is.good() );
		index_.push_back(idx);
		assert(idx == base_.size());
	}

	auto get() const -> string const&
	{
		return base_;
	}

	auto lineRange(int i) const -> std::pair<size_t, size_t>
	{
		if ( 0 <= i && static_cast<size_t>(i) + 1 < index_.size() ) {
			return std::make_pair(index_[i], index_[i + 1]);
		} else {
			return std::make_pair(index_.back(), index_.back());
		}
	}

	auto line(int i) const -> string
	{
		auto it = get().begin();
		auto ran = lineRange(i);
		return string(it + ran.first, it + ran.second);
	}
};

#endif
