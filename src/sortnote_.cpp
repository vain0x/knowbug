/*------------------------------------------------------------*/
/*
		Sort Routines (from hspda)
*/
/*------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "supio.h"

typedef struct SortData
{
    int key;
    int info;
} SORTDATA;

static SORTDATA *dtmp = NULL;

//------------------------------------------------
// 被ソートデータを交換する
//------------------------------------------------
static void swap( SORTDATA *a, SORTDATA *b )
{
    SORTDATA t;
    
    t.key   = a->key;
    t.info  = a->info;
    a->key  = b->key;
    a->info = b->info;
    b->key  = t.key;
    b->info = t.info;
    return;
}

//------------------------------------------------
// バブルソート
//------------------------------------------------
static void BubbleSortStr( SORTDATA *data, int nmem, int asdes )
{
	int i, j;
	
	for ( i = 0; i < nmem - 1; i++ ) {
		for ( j = nmem - 1; j >= i + 1; j -- ) {
			if ( asdes == 0 ) {
				if ( lstrcmp( (char *)data[j].key, (char *)data[j - 1].key) <= 0 ) {
					swap( &data[j], &data[j - 1] );
				}
			} else {
				if ( lstrcmp( (char *)data[j].key, (char *)data[j - 1].key) >= 0 ) {
					swap( &data[j], &data[j - 1] );
				}
			}
		}
	}
	return;
}

//------------------------------------------------
// 複数行文字列 -> SortData 構造体配列
//------------------------------------------------
static int NoteToData( char *adr, SORTDATA *data )
{
	int line;
	char *p;
	char a1;
	p = adr;
	line = 0;
	data[line].key  = (int)p;
	data[line].info = line;
	
	for (;;) {
		a1 = *p;
		if ( a1 == '\0' ) break;
		if ( a1 == '\r' || a1 == '\n' ) {
			*(p ++) = '\0';				// Remove CR/LF
			if ( a1 == '\r' && *p == '\n' ) p ++;
			line ++;
			data[line].key  = (int)p;
			data[line].info = line;
		} else {
			p ++;
		}
	}
	line ++;
	return line;
}

//------------------------------------------------
// 複数行文字列の行数を数える
//------------------------------------------------
static int GetNoteLines( char *adr )
{
	int line;
	char *p;
	char a1;
	p = adr;
	line = 0;
	
	for (;;) {
		a1 = *p++;
		if ( a1 == '\0' ) break;
		if ( a1 == '\n' ) line ++;
		if ( a1 == '\r' ) {
			if ( *p == '\n' ) p ++;
			line ++;
		}
	}
	line ++;
	return line;
}

//------------------------------------------------
// SortData 構造体配列 -> 複数行文字列
//------------------------------------------------
static void DataToNote( SORTDATA *data, char *adr, int num )
{
	int a;
	int len;
	char *p;
	char *s;
	p = adr;
	for ( a = 0; a < num; a ++ ) {
		s   = (char *)data[a].key;
		len = lstrlen( s );
		
		if ( len > 0 ) {
			lstrcpy( p, s );
			p += len;
			
			*(p ++) = '\r';		// Add CR/LF
			*(p ++) = '\n';	
		}
	}
	*p = '\0';
	return;
}

//------------------------------------------------
// 
//------------------------------------------------
static void DataBye( void )
{
	free(dtmp);
	dtmp = NULL;
	return;
}

//------------------------------------------------
// 
//------------------------------------------------
static void DataIni( int size )
{
	DataBye();
	dtmp = (SORTDATA *)malloc( sizeof(SORTDATA) * size );
}

//------------------------------------------------
// 複数行文字列をソートする
//------------------------------------------------
void SortNote( char *str )
{
	int i,len;
	char *p;
	char *stmp;
	
	p   = str;
	len = (int)strlen(str) + 4;
	
	i   = GetNoteLines(p);
	if ( i <= 0 ) return;
	
	DataIni( i );
	stmp = (char *)malloc( len );
	
	i = NoteToData( p, dtmp );
	BubbleSortStr( dtmp, i, 1 );
	DataToNote( dtmp, stmp, i );
	lstrcpy( p,stmp );
	
	free(stmp);
	DataBye();
	return;
}
