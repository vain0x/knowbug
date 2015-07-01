// string クラス支援モジュール

#include <cstdio>
#include "mod_cstring.h"

//------------------------------------------------
// 書式付き文字列の生成
//------------------------------------------------
string vstrf( const char* format, va_list& arglist )
{
	static char stt_tmpbuf[0x2000];
	
	// 必要な文字数を調べる
	size_t len( _vscprintf( format, arglist ) );
	
	if ( len < 0x2000 ) {
		vsprintf_s( stt_tmpbuf, format, arglist );
		
	// 必要なだけ動的に確保する
	} else {
		size_t size( len + 4 );
		char* pBuf = (char*)std::malloc( sizeof(char) * size );
		
		vsprintf_s( pBuf, size, format, arglist );
		string sResult( pBuf );
		
		std::free( pBuf );
		return sResult;
	}
	
	return stt_tmpbuf;
}

string strf( const char* format, ... )
{
	va_list   arglist;
	va_start( arglist, format );
	
	string sResult( vstrf( format, arglist ) );
	
	va_end( arglist );
	return sResult;
}

//------------------------------------------------
// 文字列の反復
//------------------------------------------------
string operator * ( const string& self, int n )
{
	string newOne = self.c_str();
	newOne *= n;
	return newOne;
}

#if 0

string& operator *= ( string& self, int n )
{
	if ( n <= 0 ) {
		self.clear();
		
	} else if ( n == 1 ) {
		//
		
	} else if ( (n & 1) == 0 ) {	// 偶数
		self += self;
		self *= (n / 2);
		
	} else {
		string tmp = self;
		self += self;
		self *= (n / 2);
		self += tmp;
	}
	
	return self;
}

#else

string& operator *= ( string& self, int n )
{
	string tmp = self;
	size_t len = self.length();
	
	self.clear();
	self.reserve( len * n + 1 );
	
	for ( int i = 0; i < n; ++ i ) {
		self += tmp;
	}
	
	return self;
}

#endif
