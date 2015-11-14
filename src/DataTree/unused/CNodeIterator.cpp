// visitor - ノード外部反復子

#include <vector>
#include "Node.h"
#include "CNodeIterator.h"

namespace DataTree
{

//##############################################################################
//                定義部 : CNodeIterator
//##############################################################################

//**********************************************************
//    メンバ変数など
//**********************************************************
struct CNodeIterator::Impl
{
public:
	typedef CNodeIterator::elem_t elem_t;
	typedef std::vector<elem_t>   children_t;
	typedef children_t::iterator  childrenIter_t;
	
public:
	children_t     children;
	childrenIter_t iter;
	
public:
//	Impl() { }
	
public:
	elem_t get(void) const
	{
		return ( (iter == children.end()) ? NULL : *iter );
	}
};

//**********************************************************
//    構築と解体
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CNodeIterator::CNodeIterator( elem_t pRoot )
	: m( new Impl )
	, mpRoot( pRoot )
	, mpElem( NULL )
{
	if ( pRoot ) {
		initialize();
		moveToBegin();
	} else {
		moveToEnd();
	}
	return;
}

//------------------------------------------------
// 解体
//------------------------------------------------
CNodeIterator::~CNodeIterator()
{ }

//------------------------------------------------
// 初期化
//------------------------------------------------
void CNodeIterator::initialize()
{
	visit( mpRoot );
	return;
}

//**********************************************************
//    Visitor としての関数
//**********************************************************
//------------------------------------------------
// 訪問 (実装)
//------------------------------------------------
void CNodeIterator::visit_impl( ITree const* pNode )
{
	pNode->acceptVisitor( const_cast<CNodeIterator*>(this) );
	return;
}

//------------------------------------------------
// 行き
//------------------------------------------------
void CNodeIterator::procPre( ITree const* pNode )
{
	if ( !isProcable() ) return;
	
	m->children.push_back( pNode );
	return;
}

//**********************************************************
//    Iterator としての関数
//**********************************************************

//------------------------------------------------
// 先頭へ移動
//------------------------------------------------
void CNodeIterator::moveToBegin(void)
{
	m->iter = m->children.begin();
	mpElem = m->get();
	return;
}

//------------------------------------------------
// 次へ移動
// 
// @ End で ++ するのを防止
//------------------------------------------------
void CNodeIterator::moveToNext(void)
{
	if ( mpElem != NULL ) {
		++ m->iter;
		mpElem = m->get();
	}
	return;
}

//------------------------------------------------
// 終端へ移動 ( 脱参照できなくなる )
//------------------------------------------------
void CNodeIterator::moveToEnd(void)
{
//	m->iter = m->children.end();
	mpElem = NULL;
	return;
}

//##############################################################################
//                静的メンバ関数
//##############################################################################
//------------------------------------------------
// 先頭の反復子を得る
//------------------------------------------------
CNodeIterator CNodeIterator::Begin( ITree const* pNode )
{
	return self_t( pNode );
}

//------------------------------------------------
// 末尾の反復子を得る
//------------------------------------------------
CNodeIterator CNodeIterator::End( ITree const* pNode )
{
	self_t obj( pNode );
	obj.moveToEnd();
	return obj;
}

}
