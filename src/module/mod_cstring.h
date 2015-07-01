// string クラス支援モジュール

// @ 「再入可能」は保障しない。

#ifndef IG_MODULE_CLASS_STRING_H
#define IG_MODULE_CLASS_STRING_H

#include <cstdarg>
#include <string>

typedef std::string CString;
typedef std::string string;

//**********************************************************
//    書式文字列の生成
//**********************************************************
extern string  strf( const char* format, ... );
extern string vstrf( const char* format, va_list& arglist );

//**********************************************************
//    文字列の反復
//**********************************************************
string  operator *  ( const string& self, int n );
string& operator *= (       string& self, int n );

#endif
