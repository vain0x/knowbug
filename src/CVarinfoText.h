// 変数データテキスト生成クラス

#ifndef IG_CLASS_VARINFO_TEXT_H
#define IG_CLASS_VARINFO_TEXT_H

#include "ClhspDebugInfo.h"
#include "module/mod_cstring.h"

//##############################################################################
//                宣言部 : CVarinfoText
//##############################################################################
class CVarinfoText
{
	//******************************************************
	//    メンバ変数
	//******************************************************
private:
	DebugInfo&  mdbginfo;
	CString    *mpBuf;
	
	PVal       *mpVar;
	const char *mpName;
	
	int mlenLimit;
	
	//******************************************************
	//    メンバ関数
	//******************************************************
public:
	explicit CVarinfoText( DebugInfo& dbginfo, int lenLimit = (0x7FFFFFFF - 1) );
	~CVarinfoText();
	
	const CString& getString(void) const
	{
		return *mpBuf;
	}
	
	void addVar( PVal *pval, const char *name );
	void addSysvar( const char *name );
#ifdef with_WrapCall
	void addCall( STRUCTDAT *pStDat, void *prmstk, int sublev, const char *name );
	void addResult( STRUCTDAT *pStDat, void *ptr, int flag, int sublev, const char *name );
	void addResult2( const CString& text, const char *name );
#endif
	
private:
	// テキストの生成
	void make( void );
	void dumpVar( PVal *pval );
	void dump( void *mem, size_t size );
	
	// 項目の追加
//	void addItem( const char *name, const char *string );
//	void addItem( const char *name, const char *format, ... );
	
	// 文字列の連結
	void cat ( const char *string );
	void catf( const char *format, ... );
	void cat_crlf( void );
	
	// その他
	
	//******************************************************
	//    封印
	//******************************************************
private:
	CVarinfoText();
	
};

#endif
