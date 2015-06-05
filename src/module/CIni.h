// INIファイル読み書きクラス

#ifndef IG_CLASS_INI_H
#define IG_CLASS_INI_H

#include <vector>
#include <string>
#include <memory>

class CIni
{
private:
	static size_t const stc_defaultBufSize = 1024;

private:
	std::string const fileName_ ;
	mutable std::vector<char> buf_;

public:
	explicit CIni(char const* fname);

	bool getBool(char const* sec, char const* key, bool defval = false) const;
	int  getInt(char const* sec, char const* key, int defval = 0) const;
	char const* getString(char const* sec, char const* key, char const* defval = "", size_t size = 0) const;

	void setBool(char const* sec, char const* key, bool val = false);
	void setInt(char const* sec, char const* key, int val, int radix = 10);
	void setString(char const* sec, char const* key, char const* val);

	std::vector<std::string> enumSections() const;
	std::vector<std::string> enumKeys(char const* sec) const;

	void removeSection(char const* sec);
	void removeKey(char const* sec, char const* key);
	bool existsKey(char const* sec, char const* key) const;
private:
	char* buf() const { return buf_.data(); };
	std::vector<std::string> enumImpl(char const* secOrNull) const;

private:
	CIni(CIni const& obj) = delete;
};

#endif
