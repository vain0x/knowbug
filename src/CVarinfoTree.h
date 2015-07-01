// 変数データのツリー形式文字列

#ifndef IG_CLASS_VARINFO_TREE_H
#define IG_CLASS_VARINFO_TREE_H

#include "ClhspDebugInfo.h"
#include "module/mod_cstring.h"

#if defined(with_Assoc) || defined(with_Vector) || defined(with_Array)
class CAssoc;
class CVector;
class CArray;
#endif

//##############################################################################
//                宣言部 : CVarinfoTree
//##############################################################################
class CVarinfoTree
{
private:
	static const int stc_cntUnitIndent = 1;		// 字下げ単位量
	
	struct BaseData
	{
		CString sIndent;
		const char* name;
		const char* indent;
		
		BaseData( const char* name_, const CString& sIndent_ )
			: name    ( name_ )
			, sIndent ( sIndent_ )
			, indent  ( sIndent.c_str() )
		{ }
		
		const char* getName  ( void ) const { return name; }
		const char* getIndent( void ) const { return sIndent.c_str(); }
		
	};
	
	//******************************************************
	//    メンバ変数
	//******************************************************
private:
	DebugInfo& mdbginfo;
	CString*   mpBuf;
	int mlvNest;
	
	int mlenLimit;	// 表示データの制限
	
	//******************************************************
	//    メンバ関数
	//******************************************************
public:
	explicit CVarinfoTree( DebugInfo& dbginfo, int lenLimit = (0x7FFFFFFF - 1) );
	~CVarinfoTree();
	
	void addVar( PVal* pval, const char* name );
	void addSysvar( int idx, const char* name, void** ppDumped, size_t* pSizeToDump );
	void addModInst( ModInst* mv, const char* name );
	void addFlexValue( FlexValue* fv, const char* name );
	void addCall( STRUCTDAT* stdat, void* prmstk, const char* name );
	void addResult( void* ptr, vartype_t type, const char* name );
	const CString& getString(void) const
	{
		return *mpBuf;
	}
	
private:
	// 項目の追加
	void addItem_value    ( const BaseData& base, vartype_t type, void* ptr );
	void addItem_var      ( const BaseData& base, PVal* pval );
	void addItem_varScalar( const BaseData& base, PVal* pval );
	void addItem_varScalar( const BaseData& base, PVal* pval, APTR aptr );
	void addItem_varArray ( const BaseData& base, PVal* pval );
	void addItem_varStr   ( const BaseData& base, PVal* pval, bool bScalar );
#ifdef clhsp
	void addItem_vector   ( const BaseData& base, Vector* vec, int length );
	void addItem_vecelem  ( const BaseData& base, VecElem* vecelem, int idx );
	void addItem_modinst  ( const BaseData& base, ModInst* mv );
#else
	void addItem_flexValue( const BaseData& base, FlexValue* fv );
#endif
	void addItem_prmstack ( const BaseData& base, STRUCTDAT* stdat, STRUCTPRM* stprm, const void* prmstack );
	void addItem_member   ( const BaseData& base, STRUCTDAT* stdat, STRUCTPRM* stprm, const void* member );
#ifdef with_Assoc
	void addItem_assoc    ( const BaseData& base, CAssoc* src );
#endif
#ifdef with_Vector
	void addItem_vector   ( const BaseData& base, CVector* src );
#endif
#ifdef with_Array
	void addItem_array    ( const BaseData& base, CArray* src );
#endif
#ifdef with_ExtraBasics
//	template<class TNumeric> CString dbgstr_extraBasic( const BaseData& base, const TNumeric src );
#endif
//	void addItem_string   ( const BaseData& base, const char* src );
	
	// 文字列の連結
	void cat( const char* src );
	void catf( const char* format, ... );
	void cat_crlf( void );
private:
	void cat( const char* src, size_t len );
	
public:
	// その他
	CString getIndent(void) const
	{
		return CString( mlvNest*  stc_cntUnitIndent, '\t' );
	}
	
	// デバッグ文字列の生成
#ifndef clhsp
	CString getDbgString( vartype_t type, const void* ptr );
	CString toStringLiteralFormat( const char* src );
#endif
	
	//******************************************************
	//    封印
	//******************************************************
private:
	CVarinfoTree();
	CVarinfoTree( const CVarinfoTree& );
	
};

extern CString makeArrayIndexString(uint dim, uint const indexes[]);

#endif
