// SysvarData を使用する関数

#include <cstring>
#include "SysvarData.h"

//------------------------------------------------
// システム変数名からインデックス値を求める
// 
// @result: sysvar-idx, or -1 (error name)
//------------------------------------------------
int getSysvarIndex( const char* name )
{
	for ( int i = 0; i < SysvarCount; ++ i ) {
		if ( !strcmp( name, SysvarData[i].name ) ) return i;
	}
	return -1;
}
