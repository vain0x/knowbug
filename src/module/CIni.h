// INIファイル読み書きクラス

#ifndef IG_CLASS_INI_H
#define IG_CLASS_INI_H

#include <vector>
#include <string>
#include <memory>

class CIni
{
private:
	static auto const stc_defaultBufSize = size_t { 1024 };

private:
	std::string const fileName_ ;
	mutable std::vector<char> buf_;

public:
	explicit CIni(char const* fname);

	bool getBool(char const* sec, char const* key, bool defval = false) const;
	auto getInt(char const* sec, char const* key, int defval = 0) const -> int;
	auto getString(char const* sec, char const* key
		, char const* defval = "", size_t size = 0
		) const -> char const*;

	void setBool(char const* sec, char const* key, bool val = false);
	void setInt(char const* sec, char const* key, int val, int radix = 10);
	void setString(char const* sec, char const* key, char const* val);

	auto enumSections()            const -> std::vector<std::string>;
	auto enumKeys(char const* sec) const -> std::vector<std::string>;

	void removeSection(char const* sec);
	void removeKey(char const* sec, char const* key);
	bool existsKey(char const* sec, char const* key) const;
private:
	auto buf() const -> char* { return buf_.data(); };
	auto enumImpl(char const* secOrNull) const -> std::vector<std::string>;

private:
	CIni(CIni const& obj) = delete;
};

#endif
