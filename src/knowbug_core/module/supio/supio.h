
//
//	supio.cpp functions
//
#define HSPAPICHAR wchar_t
#define HSPCHAR char
HSPAPICHAR *chartoapichar(const char*, HSPAPICHAR**);
void freehac(HSPAPICHAR**);
HSPCHAR *apichartohspchar(const HSPAPICHAR*, HSPCHAR**);
void freehc(HSPCHAR**);

char *mem_ini( int size );
void mem_bye( void *ptr );
int mem_save( char *fname, void *mem, int msize, int seekofs );
void strcase( char *str );
int strcpy2( char *str1, char *str2 );
int strcat2( char *str1, char *str2 );
char *strstr2( char *target, char *src );
char *strchr2( char *target, char code );
void getpath( char *stmp, char *outbuf, int p2 );
int makedir( char *name );
int changedir( char *name );
int delfile( char *name );
int dirlist( char *fname, char **target, int p3 );
int gettime( int index );
void strsp_ini( void );
int strsp_getptr( void );
int strsp_get( char *srcstr, char *dststr, char splitchr, int len );
int GetLimit( int num, int min, int max );

void Alert( char *mes );
void AlertV( char *mes, int val );
void Alertf( char *format, ... );

