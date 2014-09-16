// 仮引数情報クラス

#ifndef IG_CLASS_PARAMETER_INFORMATION_H
#define IG_CLASS_PARAMETER_INFORMATION_H

/**
@summary:
	仮引数情報を管理する。
	何を呼び出すかは知らない。CCall に所持・使用される。
	cmd_sub.cpp, CBound などが、呼び出し先と対にして生成・管理する。
**/

#include <vector>
#include "cmd_call.h"

//------------------------------------------------
// 仮引数リストの特殊タイプ
// 
// @ HSPVAR_FLAG_* と併用。
// @ _NONE を除き、( PRM_TYPE_* < 0 )。
//------------------------------------------------
int const PRM_TYPE_NONE   = ( 0);
int const PRM_TYPE_FLEX   = (-1);	// 可変長引数
int const PRM_TYPE_VAR    = (-2);	// var   指定 ( 参照渡し要求 )
int const PRM_TYPE_ARRAY  = (-3);	// array 指定 ( 参照渡し要求 )
int const PRM_TYPE_MODVAR = (-4);	// modvar指定 ( 参照渡し要求 )
int const PRM_TYPE_ANY    = (-5);	// any   指定
int const PRM_TYPE_LOCAL  = (-6);	// local 指定 ( 引数不要 )

extern bool PrmType_IsRef(int prmtype);
extern int PrmType_Size(int prmtype);
extern int PrmType_FromMPType(int mptype);

//##############################################################################
//                宣言部 : CPrmInfo
//##############################################################################
class CPrmInfo
{
public:
	using prmlist_t = std::vector<int>;

	static CPrmInfo const undeclaredFunc;	// 未宣言関数の仮引数
	static CPrmInfo const noprmFunc;		// 引数なし関数の仮引数

	//############################################
	//    メンバ変数
	//############################################
private:
	size_t mcntPrms;		// 引数の数( 最小 )
	size_t mcntLocals;		// ローカル変数の個数
	bool mbFlex;			// 可変長引数か
	prmlist_t mprmlist;		// prmtype の配列

	//############################################
	//    メンバ関数
	//############################################
public:
	//--------------------------------------------
	// 構築
	//--------------------------------------------
	explicit CPrmInfo( prmlist_t const* pPrmlist = nullptr, bool bFlex = false );
	CPrmInfo( CPrmInfo const& src ) { this->copy( src ); }

	~CPrmInfo() = default;

	//--------------------------------------------
	// 設定系
	//--------------------------------------------
	void    setFlex(bool bFlex);
	void setPrmlist(prmlist_t const& prmlist);

	//--------------------------------------------
	// 取得系
	//--------------------------------------------
	size_t cntPrms() const;
	size_t cntLocals() const;
	bool   isFlex() const;
	int  getPrmType( size_t idx ) const;
	int  getStackSize() const;							// prmstack で必要となるサイズ (可変長引数を除く)
	int  getStackSizeWithFlex( size_t cntFlex ) const;	// 〃 (可変長引数込み)

	//--------------------------------------------
	// その他
	//--------------------------------------------
	PVal* getDefaultArg( size_t iArg ) const;
	void checkCorrectArg( PVal const* pvArg, size_t iArg, bool bByRef = false ) const;

	//--------------------------------------------
	// 演算子オーバーロード
	//--------------------------------------------
	CPrmInfo& operator = ( CPrmInfo const& src ) { return this->copy( src ); }

	int compare( CPrmInfo const& rhs ) const;
	bool operator == ( CPrmInfo const& rhs ) const { return compare( rhs ) == 0; }
	bool operator != ( CPrmInfo const& rhs ) const { return !( *this == rhs ); }

	//############################################
	//    内部メンバ関数
	//############################################
private:
	CPrmInfo& copy( CPrmInfo const& obj );

	// その他
public:
	static CPrmInfo Create(hpimod::stdat_t);
};

#endif
