// CArgData - header
#if 0
#ifndef IG_CLASS_ARGDATA_H
#define IG_CLASS_ARGDATA_H

#include <vector>
#include "hsp3plugin_custom.h"

#include "CCall.h"

//##############################################################################
//               宣言部 : CArgData
//##############################################################################
// ラベル命令・関数に与えられる引数の情報
class CCall::CArgData
{
	//############################################
	//    メンバ変数
	//############################################
private:
	CCall* mpCall;					// owner

	int mCntArg;					// 引数の数
	std::vector<PVal*> mArgVal;		// 引数の値 or 参照、あるいは nullptr(nobind を表す)
	std::vector<APTR>  mIdxRef;		// 参照渡しの aptr 値 ( 値渡しなら負数 )
	std::vector<PVal*>* mpLocals;	// ローカル変数群 (prmstack 上へのポインタ; 不要なときは new しない)

	std::vector<PVal*>* const mpArgVal;	// ポインタ経由で参照することにする (今までポインタだったので)
	std::vector<APTR>*  const mpIdxRef;

	//############################################
	//    メンバ関数
	//############################################
public:
	explicit CArgData( CCall* pCall );
	CArgData( CArgData const& obj );
	~CArgData();

	// 動作系

	// 設定系
	void addArgByVal( void const* val, vartype_t vt );
	void addArgByVal( PVal* pval );
	void addArgByRef( PVal* pval ) { addArgByRef( pval, pval->offset ); }
	void addArgByRef( PVal* pval, APTR aptr );
	void addArgByVarCopy( PVal* pval );
	void addArgSkip( int lparam = 0 );
	void addLocal( PVal* pval );
	void clearArg();

	void reserveArgs( size_t cnt );
	void reserveLocals( size_t cnt );

	// 取得系
	size_t getCntArg() const { return mCntArg; }
	PVal* getArgPVal( int iArg ) const;
	APTR  getArgAptr( int iArg ) const;
	int   getArgInfo( ARGINFOID id, int iArg = -1 ) const;
	bool   isArgSkipped( int iArg ) const;
	PVal* getLocal( int iLocal ) const;

private:
	CArgData();
	CArgData& operator = ( CArgData const& obj );

	//############################################
	//    内部メンバ関数
	//############################################
private:
	CArgData& opCopy( CArgData const& obj );

	// 動作

	// 後始末
	void freeArgPVal();

};

#endif
#endif