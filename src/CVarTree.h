// VarTree

#ifndef IG_CLASS_VAR_TREE_H
#define IG_CLASS_VAR_TREE_H

#include <string>
#include <list>
#include <vector>
#include <map>
#include <iterator>

//##############################################################################
//                宣言部 : CVarTree
//##############################################################################
class CVarTree
{
	//********************************************
	//    型宣言
	//********************************************
	typedef std::string string;
	
public:
	//--------------------------------------------
	// 反復子
	//--------------------------------------------
	class iterator;
	friend class iterator;
	
	enum NodeType
	{
		NodeType_Module = 0,
		NodeType_Var
	};
	
private:
	typedef std::list<std::string>              StringList_t;
	typedef std::map<std::string, StringList_t> ModuleList_t;
	typedef std::map<std::string, CVarTree>     Children_t;
	
	typedef StringList_t::iterator StringListIter_t;
	typedef ModuleList_t::iterator ModuleListIter_t;
	typedef Children_t::iterator   ChildrenIter_t;
	
	enum PushPos
	{
		PushPos_Back  = 0,
		PushPos_Front = 1
	};
	
	//********************************************
	//    メンバ変数
	//********************************************
private:
	NodeType mType;
	string   mName;
	
	Children_t* mpChildren;
	
public:
	static string AreaName_Global;
	
	//********************************************
	//    メンバ関数
	//********************************************
public:
	CVarTree( const string& name, NodeType type );
	CVarTree( const CVarTree& src );
	~CVarTree();
	
	// 情報
	NodeType      getType() const { return mType; }
	const string& getName() const { return mName; }
	
	// 要素の追加
	void push_child ( const char* name, NodeType type );
	void push_var   ( const char* name ) { push_child( name, NodeType_Var ); }
	void push_module( const char* name ) { getChildModule( name ); }
	
	// 反復子の生成
	iterator begin() const;
	iterator   end() const;
	
private:
	ChildrenIter_t getChildModule( const char* pModname );
	
	void add( const char* name, NodeType type );
	
	//********************************************
	//    反復子のために公開 (危険)
	//********************************************
private:
	ChildrenIter_t beginChildren() const { return mpChildren->begin(); }
	ChildrenIter_t   endChildren() const { return mpChildren->end(); }
	
};

//##############################################################################
//                宣言部 : CVarTree::iterator
//##############################################################################
class CVarTree::iterator
	: public std::iterator<std::forward_iterator_tag, CVarTree, void>
{
	friend class CVarTree;
	
private:
	typedef CVarTree Elem_t;
	typedef CVarTree::ChildrenIter_t iter_t;
	typedef CVarTree::iterator       self_t;
	
	//********************************************
	//    メンバ変数
	//********************************************
private:
	CVarTree* mpOwner;
	iter_t  mIter;		// 実体
	
	Elem_t* mpData;
	
public:
	//--------------------------------------------
	// 構築 (通常)
	// 
	// @ 先頭(begin)に設定される
	//--------------------------------------------
	iterator( CVarTree* pOwner )
		: mpOwner( pOwner )
		, mpData ( nullptr )
	{
		moveToBegin();
		return;
	}
	
	//--------------------------------------------
	// 構築 (複写)
	//--------------------------------------------
	iterator( const self_t& src )
		: mpOwner( src.mpOwner )
		, mIter  ( src.mIter   )
		, mpData ( src.mpData  )
	{ }
	
	/*
	//-----------------------------------------------
	// 構築 (終端位置)
	//-----------------------------------------------
	iterator( CVarTree* pOwner, int n )
		: 
	{ }
	//*/
	
	//--------------------------------------------
	// 脱参照
	//--------------------------------------------
	Elem_t& operator *(void) const
	{
		return *mpData;
	}
	
	Elem_t* operator ->(void)
	{
		return mpData;
	}
	
	//--------------------------------------------
	// 移動操作
	//--------------------------------------------
	// 前置
	self_t& operator ++(void)
	{
		moveToNext();
		return *this;
	}
	
	// 後置
	self_t operator ++(int)
	{
		self_t obj_bak( *this );		// 現在の位置を保存しておく
		moveToNext();
		return obj_bak;
	}
	
	self_t operator +( unsigned int n )
	{
		self_t obj( *this );
		for ( unsigned int i = 0; i < n; ++ i ) obj.moveToNext();
		return obj;
	}
	
	//--------------------------------------------
	// 比較演算子
	//--------------------------------------------
	const bool operator ==(const self_t& iter) const
	{
		return ( mpData == iter.mpData );
	}
	
	const bool operator !=(const self_t& iter) const
	{
		return !(*this == iter);
	}
	
private:
	//--------------------------------------------
	// 次に移動する
	//--------------------------------------------
	void moveToNext()
	{
		// 終端に来ていたら
		if ( mIter == mpOwner->endChildren() ) {
			mpData = nullptr;
			return;
		}
		
		// 次に移動する
		mpData = &(mIter->second);
		
		++ mIter;
		
		return;
	}
	
	//------------------------------------------------
	// 先頭(begin)に移動する
	//------------------------------------------------
	void moveToBegin()
	{
		if ( mpOwner->getType() == CVarTree::NodeType_Module ) {
			mIter  = mpOwner->beginChildren();
			mpData = nullptr;
			
			moveToNext();	// 初期操作
			
		} else {
			mpData = nullptr;
		}
		return;
	}
	
	//------------------------------------------------
	// 終端(end)に移動する
	//------------------------------------------------
	void moveToEnd()
	{
		mpData = nullptr;
		return;
	}
	
	//############################################
	//    封印
	//############################################
private:
	iterator();
	
};

#endif
