// INIファイル読み書きクラス

#ifndef IG_CLASS_INI_H
#define IG_CLASS_INI_H

#include <list>
#include <string>

class CIni
{
private:
	static size_t const stc_defaultBufSize = 1024;

private:
	char* msFilename;
	char* msBuf;
	size_t msizeBuf;

public:
	explicit CIni(char const* fname = nullptr);
	~CIni();

	void open(char const* fname);
	void close();

	bool getBool(char const* sec, char const* key, bool defval = false);
	int  getInt(char const* sec, char const* key, int defval = 0);
	char const* getString(char const* sec, char const* key, char const* defval = "", size_t size = 0);

	void setBool(char const* sec, char const* key, bool val = false);
	void setInt(char const* sec, char const* key, int val, int radix = 10);
	void setString(char const* sec, char const* key, char const* val);

	std::list<std::string> enumSections();
	std::list<std::string> enumKeys(char const* sec);

	void removeSection(char const* sec);
	void removeKey(char const* sec, char const* key);
	bool existsKey(char const* sec, char const* key);

	void reserve(size_t newSize);
	void expand(size_t expandSize) { reserve(msizeBuf + expandSize); }

private:
	std::list<std::string> enumImpl(char const* secOrNull);

	// 封印
private:
	CIni(CIni const& obj) = delete;
};

//##############################################################################
//                サンプル
//##############################################################################
#if 0

#include <iostream>

template<class T>
T test()
{
	CIni ini( "./config.ini" );
	
	ini.setInt( "Resource/int", "maxnum", 1200 );
	ini.setString( "Resource/str", "text", "These are pens." );
	ini.setString( "Resource/str/word", "pen", "名,pens,ペン" );
	
	std::cout << "exists \"maxnum\" : " << ini.existsKey( "Resource/int", "maxnum" ) << std::endl;
	std::cout << "exists \"minnum\" : " << ini.existsKey( "Resource/int", "minnum" ) << std::endl;
	
	return T();
}

#define RUN_SAMPLE

#endif

#endif
