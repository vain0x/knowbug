
//
//	supio.cpp functions
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <direct.h>
#include <tchar.h>
#include "supio.h"

#ifdef _UNICODE
//
//		API用の文字エンコードへ変換
//
HSPAPICHAR *chartoapichar(const char *orig, HSPAPICHAR **pphac)
{

	int reslen;
	wchar_t *resw;
	if (orig == 0) {
		*pphac = 0;
		return 0;
	}
	reslen = MultiByteToWideChar(CP_UTF8, 0, orig, -1, (LPWSTR)NULL, 0);
	resw = (wchar_t*)calloc(reslen + 1, sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, orig, -1, resw, reslen);
	*pphac = resw;
	return resw;
}

void freehac(HSPAPICHAR **pphac)
{
	free(*pphac);
	*pphac = 0;
}

HSPCHAR *apichartohspchar(const HSPAPICHAR *orig, HSPCHAR **pphc)
{
	int plen;
	HSPCHAR *p = 0;
	if (orig == 0) {
		*pphc = 0;
		return 0;
	}
	plen = WideCharToMultiByte(CP_UTF8, NULL, orig, -1, NULL, 0, NULL, NULL);
	p = (HSPCHAR *)calloc(plen + 1, sizeof(HSPCHAR*));
	WideCharToMultiByte(CP_UTF8, NULL, orig, -1, p, plen, NULL, NULL);
	*pphc = p;
	return p;
}

void freehc(HSPCHAR **pphc)
{
	free(*pphc);
	*pphc = 0;
}

//
//		basic C I/O support
//
static FILE *fp;

char *mem_ini( int size ) {
	return (char *)calloc(size,1);
}

void mem_bye( void *ptr ) {
	free(ptr);
}


int mem_save(char *fname8, void *mem, int msize, int seekofs)
{
	FILE *fp;
	int flen;
	HSPAPICHAR *fnamew = 0;

	if (seekofs<0) {
		fp = _tfopen(chartoapichar(fname8, &fnamew), TEXT("wb"));
	}
	else {
		fp = _tfopen(chartoapichar(fname8, &fnamew), TEXT("r+b"));
	}
	freehac(&fnamew);
	if (fp == NULL) return -1;
	if (seekofs >= 0) fseek(fp, seekofs, SEEK_SET);
	flen = (int)fwrite(mem, 1, msize, fp);
	fclose(fp);
	free(fnamew);
	return flen;
}


/*
int dpm_read( char *fname, void *readmem, int rlen, int seekofs )
{
	char *lpRd;
	FILE *ff;
	int a1;
	int seeksize;

	seeksize=seekofs;
	if (seeksize<0) seeksize=0;

	lpRd=(char *)readmem;

	//	Read normal file

	ff = fopen( fname, "rb" );
	if ( ff == NULL ) return -1;
	if ( seekofs>=0 ) fseek( ff, seeksize, SEEK_SET );
	a1 = (int)fread( lpRd, 1, rlen, ff );
	fclose( ff );
	return a1;
}


int dpm_fread( void *mem, int size, FILE *stream )
{
	int len;
	len = (int)fread( mem, 1, size, stream );
	return len;
}


int dpm_exist( char *fname )
{
	FILE *ff;
	int length;

	ff=fopen( fname,"rb" );
	if (ff==NULL) return -1;
	fseek( ff,0,SEEK_END );
	length=(int)ftell( ff );			// normal file size
	fclose(ff);

	return length;
}


FILE *dpm_open( char *fname )
{
	fp=fopen( fname,"rb" );
	return fp;
}

void dpm_close()
{
	fclose(fp);
}


int dpm_filecopy( char *fname, char *sname )
{
	FILE *fp;
	FILE *fp2;
	int flen;
	int max=0x8000;
	int rval;
	char *mem;

	rval=1;
	mem=(char *)malloc(max);
	fp=fopen(fname,"rb");if (fp==NULL) goto jobov;
	fp2=fopen(sname,"wb");if (fp2==NULL) goto jobov;
	while(1) {
		flen = (int)fread( mem, 1, max, fp );
		if (flen==0) break;
		fwrite( mem, 1, flen, fp2 );
		if (flen<max) break;
	}
	fclose(fp2);fclose(fp);
	rval=0;
jobov:
	free(mem);
	return rval;
}


void dpm_memfile( void *mem, int size )
{
}


char *dpm_readalloc( char *fname )
{
	char *p;
	int len;
	len = dpm_exist( fname );
	if ( len < 0 ) return NULL;
	p = mem_ini( len );
	dpm_read( fname, p, len, 0 );
	return p;
}
*/

void strcase(char *target)
{
	//		strをすべて小文字に(utf8対応版)
	//
	unsigned char *p;
	unsigned char a1;
	p = (unsigned char *)target;
	while (1) {
		a1 = *p; if (a1 == 0) break;
		*p = tolower(a1);
		p++;							// 検索位置を移動
		if (a1 >= 128) {					// 多バイト文字チェック
			if (a1 >= 192) p++;
			if (a1 >= 224) p++;
			if (a1 >= 240) p++;
			if (a1 >= 248) p++;
			if (a1 >= 252) p++;
		}
	}
}


int strcpy2( char *str1, char *str2 )
{
	//	string copy (ret:length)
	//
	char *p;
	char *src;
	char a1;
	src = str2;
	p = str1;
	while(1) {
		a1=*src++;if (a1==0) break;
		*p++=a1;
	}
	*p++=0;
	return (int)(p-str1);
}


int strcat2( char *str1, char *str2 )
{
	//	string cat (ret:length)
	//
	char *src;
	char a1;
	int i;
	src = str1;
	while(1) {
		a1=*src;if (a1==0) break;
		src++;
	}
	i = (int)(src-str1);
	return (strcpy2(src,str2)+i);
}


char *strstr2(char *target, char *src)
{
	//		strstr関数のutf8対応版
	//
	unsigned char *p;
	unsigned char *s;
	unsigned char *p2;
	unsigned char a1;
	unsigned char a2;
	unsigned char a3;
	p = (unsigned char *)target;
	if ((*src == 0) || (*target == 0)) return NULL;
	while (1) {
		a1 = *p; if (a1 == 0) break;
		p2 = p;
		s = (unsigned char *)src;
		while (1) {
			a2 = *s++; if (a2 == 0) return (char *)p;
			a3 = *p2++; if (a3 == 0) break;
			if (a2 != a3) break;
		}
		p++;							// 検索位置を移動
		if (a1 >= 128) {					// 多バイト文字チェック
			if (a1 >= 192) p++;
			if (a1 >= 224) p++;
			if (a1 >= 240) p++;
			if (a1 >= 248) p++;
			if (a1 >= 252) p++;
		}
	}
	return NULL;
}


char *strchr2(char *target, char code)
{
	//		str中最後のcode位置を探す(utf8対応版)
	//
	unsigned char *p;
	unsigned char a1;
	char *res;
	p = (unsigned char *)target;
	res = NULL;
	while (1) {
		a1 = *p; if (a1 == 0) break;
		if (a1 == code) res = (char *)p;
		p++;							// 検索位置を移動
		if (a1 >= 128) {					// 多バイト文字チェック
			if (a1 >= 192) p++;
			if (a1 >= 224) p++;
			if (a1 >= 240) p++;
			if (a1 >= 248) p++;
			if (a1 >= 252) p++;
		}
	}
	return res;
}


void getpath( char *stmp, char *outbuf, int p2 )
{
	char *p;
	char p_drive[_MAX_PATH];
	char p_dir[_MAX_DIR];
	char p_fname[_MAX_FNAME];
	char p_ext[_MAX_EXT];

	p = outbuf;
	if (p2&16) strcase( stmp );
	_splitpath( stmp, p_drive, p_dir, p_fname, p_ext );
	strcat( p_drive, p_dir );
	if ( p2&8 ) {
		strcpy( stmp, p_fname ); strcat( stmp, p_ext );
	} else if ( p2&32 ) {
		strcpy( stmp, p_drive );
	}
	switch( p2&7 ) {
	case 1:			// Name only ( without ext )
		stmp[ strlen(stmp)-strlen(p_ext) ] = 0;
		strcpy( p, stmp );
		break;
	case 2:			// Ext only
		strcpy( p, p_ext );
		break;
	default:		// Direct Copy
		strcpy( p, stmp );
		break;
	}
}


int makedir(char *name8)
{
#ifdef HSPWIN
	HSPAPICHAR *namew = 0;
	int res;

	res = _tmkdir(chartoapichar(name8, &namew));
	freehac(&namew);
	return res;
#else
	return 0;
#endif
}


int changedir(char *name8)
{
#ifdef HSPWIN
	HSPAPICHAR *namew = 0;
	int res;
	res = _tchdir(chartoapichar(name8, &namew));
	freehac(&namew);
	return res;
#else
	return 0;
#endif
}

int delfile( char *name )
{
	int res;
	HSPAPICHAR *hactmp1 = 0;
	res = DeleteFile( chartoapichar(name,&hactmp1) );
	freehac(&hactmp1);
	return res;
}


int gettime( int index )
{
/*
	Get system time entries
	index :
	    0 wYear
	    1 wMonth
	    2 wDayOfWeek
	    3 wDay
	    4 wHour
	    5 wMinute
	    6 wSecond
	    7 wMilliseconds
*/
	SYSTEMTIME st;
	short *a;
	GetLocalTime( &st );
	a=(short *)&st;
	return (int)(a[index]);
}


static	int splc;	// split pointer

void strsp_ini(void)
{
	splc = 0;
}

int strsp_getptr(void)
{
	return splc;
}

int strsp_get(char *srcstr, char *dststr, char splitchr, int len)
{
	//		split string with parameters
	//

	/*
	rev 44
	mingw : warning : 比較は常に偽
	に対処
	*/
	unsigned char a1;
	unsigned char a2;
	int a;
	int utf8cnt;
	a = 0; utf8cnt = 0;
	while (1) {
		utf8cnt = 0;
		a1 = srcstr[splc];
		if (a1 == 0) break;
		splc++;
		if (a1 >= 128) {					// 多バイト文字チェック
			if (a1 >= 192) utf8cnt++;
			if (a1 >= 224) utf8cnt++;
			if (a1 >= 240) utf8cnt++;
			if (a1 >= 248) utf8cnt++;
			if (a1 >= 252) utf8cnt++;
		}

		if (a1 == splitchr) break;
		if (a1 == 13) {
			a2 = srcstr[splc];
			if (a2 == 10) splc++;
			break;
		}
#ifdef HSPLINUX
		if (a1 == 10) {
			a2 = srcstr[splc];
			break;
		}
#endif
		dststr[a++] = a1;
		if (utf8cnt>0) {
			while (utf8cnt>0) {
				dststr[a++] = srcstr[splc++];
				utf8cnt--;
			}
		}
		if (a >= len) break;
	}
	dststr[a] = 0;
	return (int)a1;
}

char *strsp_cmds(char *srcstr)
{
	//		Skip 1parameter from command line
	//
	int spmode;
	char a1;
	char *cmdchk;
	cmdchk = srcstr;
	spmode = 0;
	while (1) {
		a1 = *cmdchk;
		if (a1 == 0) break;
		cmdchk++;
		if (a1 == 32) if (spmode == 0) break;
		if (a1 == 0x22) spmode ^= 1;
	}
	return cmdchk;
}


int GetLimit( int num, int min, int max )
{
	if ( num > max ) return max;
	if ( num < min ) return min;
	return num;
}



//
//		windows debug support
//

void Alert( char *mes )
{
	HSPAPICHAR *hactmp1 = 0;
	MessageBox( NULL, chartoapichar(mes,&hactmp1), TEXT("error"),MB_ICONINFORMATION | MB_OK );
	freehac(&hactmp1);
}

void AlertV( char *mes, int val )
{
	char ss[128];
	HSPAPICHAR *hactmp1 = 0;
	sprintf( ss, "%s%d",mes,val );
	MessageBox( NULL, chartoapichar(ss,&hactmp1), TEXT("error"),MB_ICONINFORMATION | MB_OK );
	freehac(&hactmp1);
}

void Alertf( char *format, ... )
{
	char textbf[1024];
	va_list args;
	HSPAPICHAR *hactmp1;
	va_start(args, format);
	vsprintf(textbf, format, args);
	va_end(args);
	MessageBox( NULL, chartoapichar(textbf,&hactmp1), TEXT("error"),MB_ICONINFORMATION | MB_OK );
	freehac(&hactmp1);
}
#else
#endif



