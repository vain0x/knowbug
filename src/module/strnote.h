// CStrNote

#ifndef IG_STR_NOTE_H
#define IG_STR_NOTE_H

//##############################################################################
//               宣言部 : CStrNote
//##############################################################################
class CStrNote
{
	//******************************************************
	//        メンバ関数
	//******************************************************
public:
	CStrNote();
	~CStrNote();
	
	void  Select( char* str );
	int   GetSize(void);
	char* GetStr(void);
	int   GetMaxLine(void);
	int   GetLine( char* nres, int line );
	int   GetLine( char* nres, int line, int max );
	int   PutLine( char* nstr, int line, int ovr );
	char* GetLineDirect( int line );
	void  ResumeLineDirect(void);
	
private:
	int nnget( char* nbase, int line );
	
	//******************************************************
	//        メンバ変数
	//******************************************************
private:
	char* base;
	int lastcr;
	char* nn;
	char* lastnn;
	char lastcode;
	char nulltmp[4];
};

//##############################################################################
//               宣言部 : CStrLineIterator
//##############################################################################
class CStrLineIterator
{
	//******************************************************
	//        メンバ関数
	//******************************************************
public:
	explicit CStrLineIterator( char* str );
	~CStrLineIterator();
	
	bool hasNext(void) const;
	int next( char* dest, int len );
	
	//******************************************************
	//        メンバ変数
	//******************************************************
private:
	char* mStrBuf;
	int   mIndex;
};

#endif
