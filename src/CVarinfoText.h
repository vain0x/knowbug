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
	static const char *getModeString( varmode_t mode )
	{
		return	( mode <= HSPVAR_MODE_NONE   ) ? "無効" :
				( mode == HSPVAR_MODE_MALLOC ) ? "実体" :
				( mode == HSPVAR_MODE_CLONE  ) ? "クローン" : "???"
		;
	}
	
	//******************************************************
	//    封印
	//******************************************************
private:
	CVarinfoText();
	
};

#endif
