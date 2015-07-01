
/*----------------------------------------------------------------*/
//		notepad object related routines
//		(Linux対応のためCR/LFだけでなくLFにも対応した版)
/*----------------------------------------------------------------*/

#include <string.h>
#include "strnote.h"

//**********************************************************
//        Interfaces
//**********************************************************

//------------------------------------------------
// 構築
//------------------------------------------------
CStrNote::CStrNote()
{
	base       = NULL;
	nulltmp[0] = 0;
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CStrNote::~CStrNote()
{
	return;
}

//------------------------------------------------
// 文字列バッファを選択(設定)する
//------------------------------------------------
void CStrNote::Select( char* str )
{
	base = str;
	return;
}

//------------------------------------------------
// 総バイト数を得る
//------------------------------------------------
int CStrNote::GetSize(void)
{
	return static_cast<int>( strlen( base ) );
}

//-------------------------------------------------------------
//		Routines
//-------------------------------------------------------------
int CStrNote::nnget( char* nbase, int line )
{
	int a, i;
	char a1;
	a      = 0;
	lastcr = 0;
	nn     = nbase;
	if ( line < 0 ) {
		i = static_cast<int>( strlen(nbase) );
		if ( i == 0 ) return 0;
		nn += i;
		a1  = nn[-1];
		if ( a1 == '\r' || a1 == '\n' ) lastcr ++;
		return 0;
	}
	
	if ( line ) {
		for (;;) {
			a1 = *nn;
			if ( a1 == 0 ) return 1;
			nn ++;
#ifdef HSPLINUX
			if ( a1 == '\n' ) {
				a ++;
				if ( a == line ) break;
			}
#endif
			if ( a1 == '\r' ) {
				if ( *nn == '\n' ) nn ++;
				a ++;
				if ( a == line ) break;
			}
		}
	}
	lastcr ++;
	return 0;
}

//------------------------------------------------
// Get specified line from note
// 
// @result: 0 = ok, 1 = no line
//------------------------------------------------
int CStrNote::GetLine( char* nres, int line )
{
	char a1;
	char* pp = nres;
	if ( nnget( base, line ) ) return 1;
	if ( *nn == '\0' ) return 1;
	
	for (;;) {
		a1 = *(nn ++);
		if ( a1 == '\0' || a1 == '\r' ) break;
#ifdef HSPLINUX
		if ( a1 == '\n' ) break;
#endif
		*(pp ++) = a1;
	}
	*pp = '\0';
	return 0;
}

//------------------------------------------------
// Get specified line from note
// 
// @result : 0 = ok, 1 = no line
//------------------------------------------------
int CStrNote::GetLine( char* nres, int line, int max )
{
	char a1;
	char* pp;
	int cnt;
	pp  = nres;
	cnt = 0;
	if ( nnget( base, line ) ) return 1;
	if ( *nn == '\0' ) return 1;
	
	for (;;) {
		if ( cnt >= max ) break;
		a1 = *(nn ++);
		if ( a1 == '\0' || a1 == '\r' ) break;
#ifdef HSPLINUX
		if ( a1 == '\n' ) break;
#endif
		*(pp ++) = a1;
		cnt ++;
	}
	*pp = '\0';
	return 0;
}

//------------------------------------------------
// Get specified line from note
//------------------------------------------------
char*  CStrNote::GetLineDirect( int line )
{
	char a1;
	if ( nnget( base, line ) ) nn = nulltmp;
	lastnn = nn;
	
	for (;;) {
		a1 = *lastnn;
		if ( a1 == '\0' || a1 == '\r' ) break;
#ifdef HSPLINUX
		if ( a1 == '\n' ) break;
#endif
		lastnn ++;
	}
	
	lastcode = a1;
	*lastnn  = '\0';
	return nn;
}

//------------------------------------------------
// Resume last GetLineDirect()
//------------------------------------------------
void CStrNote::ResumeLineDirect(void)
{
	*lastnn = lastcode;
	return;
}

//------------------------------------------------
// 行数を得る
//------------------------------------------------
int CStrNote::GetMaxLine(void)
{
	int a, b;
	char a1;
	a  = 1;
	b  = 0;
	nn = base;
	
	for (;;) {
		a1 = *(nn ++);
		if ( a1 == '\0' ) break;
#ifdef HSPLINUX
		if ( a1 == '\r' || a1 == '\n' ) {
#else
		if ( a1 == '\r' ) {
			if ( *nn == '\n' ) nn ++;
#endif
			a ++;
			b = 0;
		} else {
			b ++;
		}
	}
	if ( b == 0 ) a --;
	return a;
}

//------------------------------------------------
// Pet specified line to note
// 
// @result : 0 = ok, 1 = no line
//------------------------------------------------
int CStrNote::PutLine( char* nstr2, int line, int ovr )
{
	int a (0), ln, la, lw;
	char a1;
	char* pp;
	char* p1;
	char* p2;
	char* nstr;
	
	if ( nnget( base, line ) ) return 1;
	if ( lastcr == 0 ) {
		if ( nn != base ) {
			strcat( base, "\r\n" );
			nn += 2;
		}
	}
	nstr = nstr2;
	if ( nstr == NULL ) { nstr = ""; }
	
	pp = nstr;
	if ( nstr2 != NULL ) strcat(nstr, "\r\n");
	ln = static_cast<int>( strlen(nstr) );		// base new str + cr/lf
	la = static_cast<int>( strlen(base) );
	lw = static_cast<int>( la - (nn - base) + 1 );
	
	if ( ovr ) {				// when overwrite mode
		p1 = nn;
		a  = 0;
		
		for (;;) {
			a1 = *(p1 ++);
			if ( a1 == 0 ) break;
			a ++;
#ifdef HSPLINUX
			if ( a1 == '\r' || a1 == '\n' ) {
#else
			if ( a1 == '\r' ) {
				if ( *p1 == '\n' ) { p1 ++; a ++; }
#endif
				break;
			}
		}
		ln = ln - a;
		lw = lw - a;
		if ( lw < 1 ) lw = 1;
	}
	
	if ( ln >= 0 ) {
		p1 = base + la + ln;
		p2 = base + la;
		for( a = 0; a < lw; ++ a ) { *(p1 --) = *(p2 --); }
	} else {
		p1 = nn + a + ln;
		p2 = nn + a;
		for( a = 0; a < lw; ++ a ) { *(p1 ++) = *(p2 ++); }
	}
	
	for (;;) {
		a1 = *(pp ++);
		if ( a1 == 0 ) break;
		*(nn ++) = a1;
	}
	return 0;
}

//##############################################################################
//               定義部 : CStrLineIterator
//##############################################################################
//------------------------------------------------
// 構築
//------------------------------------------------
CStrLineIterator::CStrLineIterator( char* str )
{
	mStrBuf = str;
	mIndex  = 0;
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CStrLineIterator::~CStrLineIterator()
{ }

//------------------------------------------------
// 続きがあるか
//------------------------------------------------
bool CStrLineIterator::hasNext(void) const
{
	return mStrBuf[mIndex] != '\0';
}

//------------------------------------------------
// 次の行を取り出す
// 
// @prm dst := 行の文字列の複写先
// @prm len := dst に格納できる最大の長さ
// @result  := 取り出した文字列長
// @	バッファが足りない => -1
//------------------------------------------------
int CStrLineIterator::next( char* dst, int len )
{
	int  i    = 0;
	bool over = false;
	for (;;) {
		char c = mStrBuf[mIndex];
		if ( c == '\0' ) {
			break;
		}
#ifdef HSPLINUX
		if( c == '\n' ) {
			mIndex ++;
			break;
		}
#endif
		if ( c == '\r' ) {
			mIndex ++;
			if( mStrBuf[mIndex] == '\n' ) mIndex ++;
			break;
		}
		if ( i < len ) {
			dst[i] = c;
		} else {
			over = true;
		}
		i ++;
		mIndex ++;
	}
	
	if ( over ) return -1;
	
	if ( i < len ) {
		dst[i] = '\0';
	}
	return i;
}
