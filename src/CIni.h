// INIファイル読み書きクラス

#ifndef IG_CLASS_INI_H
#define IG_CLASS_INI_H

//##############################################################################
//                宣言部 : CIni
//##############################################################################
class CIni
{
private:
	static const int stc_defaultBufSize = 1024;
	
private:
	char *msFilename;
	char *msBuf;
	size_t msizeBuf;
	
public:
	explicit CIni( const char *fname = NULL );
	~CIni();
	
	void open( const char *fname );
	void close( void );
	
	bool getBool( const char *sec, const char *key, bool defval = false );
	int  getInt ( const char *sec, const char *key, int defval = 0 );
	const char *getString( const char *sec, const char *key, const char *defval = "", size_t size = 0 );
	
	void setBool  ( const char *sec, const char *key, bool val = false );
	void setInt   ( const char *sec, const char *key, int val, int radix = 10 );
	void setString( const char *sec, const char *key, const char *val );
	
	void removeSection( const char *sec );
	void removeKey    ( const char *sec, const char *key );
	bool existsKey    ( const char *sec, const char *key );
	
	void reserve( size_t newSize );
	void expand( size_t expandSize ) { reserve( msizeBuf + expandSize ); }
	
	// 封印
private:
	CIni( const CIni& obj );
};

//##############################################################################
//                サンプル
//##############################################################################
#if 0

#include <iostream>

template<class T>
T test( void )
{
	CIni ini;
	
	ini.open( "./config.ini" );
	
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
