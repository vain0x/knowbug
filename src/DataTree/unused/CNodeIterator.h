// visitor - ノード反復子

#ifndef IG_CLASS_NODE_ITERATOR_H
#define IG_CLASS_NODE_ITERATOR_H

#include <iterator>
#include "Node.dec.h"

//#include <boost/shared_ptr.hpp>

#include "ITreeVisitor.h"

namespace DataTree
{

//##############################################################################
//                宣言部 : CNodeIterator
//##############################################################################
//------------------------------------------------
// ノード反復子
// 
// @ 外部反復子。
// @ 「参照型」クラス。複製しても同じモノを参照する。
// @	dup() で、同じ内容を持つ (だろう) 別の実体を生成できる。
// @ 生成された時点で、行き先は決定する。
// @	そのため、pRoot が変更された時点で、この反復子は無効になる。
//------------------------------------------------
class CNodeIterator
	: public std::iterator<std::forward_iterator_tag, ITree const*, void>
	//, public IFlatVisitor<ITree>	// Mix-in
{
public:
	typedef CNodeIterator self_t;
	typedef ITree const*  elem_t;
	typedef self_t this_t;
	typedef elem_t Elem_t;

	typedef elem_t target_t;// visitor の target
	
public:
	CNodeIterator( elem_t pRoot );
	CNodeIterator( self_t const& obj ) { opCopy( obj ); }
	virtual ~CNodeIterator();
	
	//******************************************************
	//    Visitor としての関数
	//******************************************************
public:
	virtual void visit_impl(target_t);
	
private:
	// 処理
	virtual void procPre(target_t);
	virtual bool requiresEach(target_t) const { return false; }
	virtual bool requiresPost(target_t) const { return false; }
	
	//******************************************************
	//    Iterator としての関数
	//******************************************************
public:
	static self_t Begin( elem_t );
	static self_t End  ( elem_t );
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//    脱参照
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	elem_t operator * () const { return mpElem; }
	elem_t operator ->() const { return mpElem; }
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//    移動
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	// 前置
	self_t& operator ++()
	{
		moveToNext();
		return *this;
	}
	
	// 後置
	self_t operator ++(int)
	{
		self_t obj_bak( dup() );
		moveToNext();
		return obj_bak;
	}
	
	// 代入
	self_t& operator =(self_t const& obj)
	{
		return opCopy( obj );
	}
	
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	//    比較
	//++++++++++++++++++++++++++++++++++++++++++++++++++++++
	bool operator ==(self_t const& obj) const
	{
		return ( mpElem == obj.mpElem );
	}
	
	bool operator !=(self_t const& obj) const
	{
		return !( *this == obj );
	}
	
	bool operator !() const
	{
		return ( mpElem == nullptr );
	}
	
private:
	void initialize();
	void moveToBegin();
	void moveToNext();
	void moveToEnd();
	
	self_t& opCopy( self_t const& obj )
	{
		mpRoot = obj.mpRoot;
		mpElem = obj.mpElem;
		m      = obj.m;
		return *this;
	}
	
	self_t dup() const
	{
		return self_t( mpRoot );
	}
	
	//******************************************************
	//    メンバ変数
	//******************************************************
private:
	elem_t mpRoot;
	elem_t mpElem;
	
	struct Impl;
	Impl* m;
};

}

#endif
