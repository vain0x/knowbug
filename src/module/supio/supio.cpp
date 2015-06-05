
//
//	supio.cpp functions
//

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <direct.h>
#include "supio.h"

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


int mem_save( char *fname, void *mem, int msize, int seekofs )
{
	FILE *fp;
	int flen;

	if (seekofs<0) {
		fp=fopen(fname,"wb");
	}
	else {
		fp=fopen(fname,"r+b");
	}
	if (fp==NULL) return -1;
	if ( seekofs>=0 ) fseek( fp, seekofs, SEEK_SET );
	flen = (int)fwrite( mem, 1, msize, fp );
	fclose(fp);
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

void strcase( char *target )
{
	//		strをすべて小文字に(全角対応版)
	//
	unsigned char *p;
	unsigned char a1;
	p=(unsigned char *)target;
	while(1) {
		a1=*p;if ( a1==0 ) break;
		*p=tolower(a1);
		p++;							// 検索位置を移動
		if (a1>=129) {					// 全角文字チェック
			if ((a1<=159)||(a1>=224)) p++;
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


char *strstr2( char *target, char *src )
{
	//		strstr関数の全角対応版
	//
	unsigned char *p;
	unsigned char *s;
	unsigned char *p2;
	unsigned char a1;
	unsigned char a2;
	unsigned char a3;
	p=(unsigned char *)target;
	if (( *src==0 )||( *target==0 )) return NULL;
	while(1) {
		a1=*p;if ( a1==0 ) break;
		p2 = p;
		s=(unsigned char *)src;
		while(1) {
			a2=*s++;if (a2==0) return (char *)p;
			a3=*p2++;if (a3==0) break;
			if (a2!=a3) break;
		}
		p++;							// 検索位置を移動
		if (a1>=129) {					// 全角文字チェック
			if ((a1<=159)||(a1>=224)) p++;
		}
	}
	return NULL;
}


char *strchr2( char *target, char code )
{
	//		str中最後のcode位置を探す(全角対応版)
	//
	unsigned char *p;
	unsigned char a1;
	char *res;
	p=(unsigned char *)target;
	res = NULL;
	while(1) {
		a1=*p;if ( a1==0 ) break;
		if ( a1==code ) res=(char *)p;
		p++;							// 検索位置を移動
		if (a1>=129) {					// 全角文字チェック
			if ((a1<=159)||(a1>=224)) p++;
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


int makedir( char *name )
{
	return _mkdir( name );
}


int changedir( char *name )
{
	return _chdir( name );
}


int delfile( char *name )
{
	return DeleteFile( name );
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

void strsp_ini( void )
{
	splc=0;
}

int strsp_getptr( void )
{
	return splc;
}

int strsp_get( char *srcstr, char *dststr, char splitchr, int len )
{
	//		split string with parameters
	//
	char a1;
	char a2;
	int a;
	int sjflg;
	a=0;sjflg=0;
	while(1) {
		sjflg=0;
		a1=srcstr[splc];
		if (a1==0) break;
		splc++;
		if (a1>=0x81) if (a1<0xa0) sjflg++;
		if (a1>=0xe0) sjflg++;

		if (a1==splitchr) break;
		if (a1==13) {
			a2=srcstr[splc];
			if (a2==10) splc++;
			break;
		}
		dststr[a++]=a1;
		if (sjflg) {
			dststr[a++]=srcstr[splc++];
		}
		if ( a>=len ) break;
	}
	dststr[a]=0;
	return (int)a1;
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
	MessageBox( NULL, mes, "error",MB_ICONINFORMATION | MB_OK );
}

void AlertV( char *mes, int val )
{
	char ss[128];
	sprintf( ss, "%s%d",mes,val );
	MessageBox( NULL, ss, "error",MB_ICONINFORMATION | MB_OK );
}

void Alertf( char *format, ... )
{
	char textbf[1024];
	va_list args;
	va_start(args, format);
	vsprintf(textbf, format, args);
	va_end(args);
	MessageBox( NULL, textbf, "error",MB_ICONINFORMATION | MB_OK );
}



