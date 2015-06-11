#ifndef IG_LINE_DELIMITED_STRING_H
#define IG_LINE_DELIMITED_STRING_H

#include <string>
#include <vector>
#include <fstream>
#include <cstring>

static size_t countIndents(char const* s)
{
	size_t i = 0;
	for ( ; s[i] == '\t' || s[i] == ' '; ++i );
	return i;
}

//行ごとに区切られた変更不可な文字列
//ただし各行の字下げは消去する
class LineDelimitedString {
	using string = std::string;

	string base_;
	std::vector<size_t> index_; //[i]: i行目の先頭の字下げ後への添字; back(): 末尾への添字

public:
	LineDelimitedString(std::istream& is)
	{
		char linebuf[0x1000];
		size_t idx = 0;
		do {
			is.getline(linebuf, sizeof(linebuf));
			index_.push_back(idx + countIndents(linebuf));

			size_t const len = std::strlen(linebuf);
			base_.append(linebuf, linebuf + len).append("\r\n");
			idx += len + 2;
		} while ( is.good() );
		index_.push_back(idx);
		assert(idx == base_.size());
	}

	string const& get() const {
		return base_;
	}
	std::pair<size_t, size_t> lineRange(int i) const {
		if ( 0 <= i && static_cast<size_t>(i) + 1 < index_.size() ) {
			return std::make_pair(index_[i], index_[i + 1]);
		} else {
			return std::make_pair(index_.back(), index_.back());
		}
	}
	string line(int i) const {
		auto const it = get().begin();
		auto&& ran = lineRange(i);
		return string(it + ran.first, it + ran.second);
	}
};

#endif
