// 変数データの一行文字列

#ifndef IG_CLASS_VARINFO_LINE_H
#define IG_CLASS_VARINFO_LINE_H

#include "ClhspDebugInfo.h"
#include "module/mod_cstring.h"

#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
class CAssoc;
class CVector;
class CArray;
#endif

//##############################################################################
//                宣言部 : CVarinfoLine
//##############################################################################
class CVarinfoLine
{
private:
	//******************************************************
	//    メンバ変数
	//******************************************************
private:
	DebugInfo& mdbginfo;
	CString   *mpBuf;
	int mlvNest;
	
	int mlenLimit;	// 表示データの制限
	
	//******************************************************
	//    メンバ関数
	//******************************************************
public:
	explicit CVarinfoLine( DebugInfo& dbginfo, int lenLimit = (0x7FFFFFFF - 1) );
	~CVarinfoLine();
	
	void addVar( PVal *pval );
	void addSysvar( int idx );
	void addModInst( ModInst *mv );
	void addFlexValue( FlexValue *fv );
	void addCall( STRUCTDAT *pStDat, void *prmstk );
	void addResult( void* ptr, vartype_t type );
	
	const CString& getString(void) const
	{
		return *mpBuf;
	}
	
	// 項目の追加
private:
	void addItem_value    ( vartype_t type, void *ptr );
	void addItem_var      ( PVal *pval );
	void addItem_varScalar( PVal *pval );
	void addItem_varScalar( PVal *pval, APTR aptr );
	void addItem_varArray ( PVal *pval );
	void addItem_varStr   ( PVal *pval, bool bScalar );
#ifdef clhsp
	void addItem_vector   ( Vector *vec, int length );
	void addItem_vecelem  ( VecElem *vecelem, int idx );
	void addItem_modinst  ( ModInst *mv );
#else
	void addItem_flexValue( FlexValue *fv );
#endif
	void addItem_prmstack ( STRUCTDAT *pStDat, STRUCTPRM *pStPrm, const void *prmstack );
	void addItem_member   ( STRUCTDAT *pStDat, STRUCTPRM *pStPrm, const void *member );
#ifdef with_Assoc
	void addItem_assoc    ( CAssoc* src );
#endif
#ifdef with_Vector
	void addItem_vector   ( CVector* src );
#endif
#ifdef with_Array
	void addItem_array    ( CArray* src );
#endif
#ifdef with_ExtraBasics
//	template<class TNumeric> CString dbgstr_extraBasic( const BaseData& base, const TNumeric src );
#endif
//	void addItem_string   ( const BaseData& base, const char *src );
	
	// 文字列の連結
public:
	void cat( const char *src, size_t len );
	void cat( const char *src );
	void catf( const char *format, ... );
	void cat_crlf();
	
public:
	// デバッグ文字列の生成
#ifndef clhsp
	CString getDbgString( vartype_t type, const void *ptr );
	CString toStringLiteralFormat( const char *src );
#endif
	
	//******************************************************
	//    封印
	//******************************************************
private:
	CVarinfoLine();
	CVarinfoLine( const CVarinfoLine& );
	
};

#endif
