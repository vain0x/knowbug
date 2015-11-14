// visitor

// ※無理だった

#ifndef IG_INTERFACE_VISITOR_H
#define IG_INTERFACE_VISITOR_H

//##############################################################################
//                宣言部 : IVisitor
//##############################################################################
//------------------------------------------------
// visitor
// 
// @ Mix-in インターフェース・クラス(Interface-class)。
// @ INode のツリーを訪れ (visit() し)、処理を行う。
// @ 呼び出しを処理しない => requires*() で false を返す。
// @	requires を再定義しない =>> 既定を用いる => 処理するとみなす。
// @ 二分木ではないので、通りがけ順での処理には対応できない。
// @ 1. visit() で訪れるべきノードを受け取る。そうしたら、渡されたノードが持つ、
// @	Visitor を受け入れる関数 (acceptVisitor()) を呼ぶ。
// @ 2. 始めに、ノードは、IVisitor::procPre() を呼ぶ。この関数で Visitor は、
// @	行きがけ順の場合に行う処理を実行する。
// @ 3. ノードは、Visitor に、子ノードすべてを訪れるように指示する。つまり、
// @	procEach() を呼び、その後、visit() に子ノードを渡す ( ここで再帰呼び出し
// @	が発生する )。
// @ 4. 最後に、ノードは、IVisitor::procPost() を呼ぶ。この関数で、Visitor は、
// @	帰りがけ順の場合に行う処理を実行する。
// @ 5. return;
//------------------------------------------------
template<class T>
class IVisitor
{
public:
	typedef const T* target_t;
	
public:
//	IVisitor();
	virtual ~IVisitor() { }
	
	//******************************************************
	//    インターフェース
	//******************************************************
public:
	virtual void visit( target_t ) = 0;
	
	// 処理 ( requires チェックのみ )
	virtual void procPre ( target_t p ) { if ( requiresPre (p) ) procImplPre (p); }
	virtual void procEach( target_t p ) { if ( requiresEach(p) ) procImplEach(p); }
	virtual void procPost( target_t p ) { if ( requiresPost(p) ) procImplPost(p); }
	
protected:
	// 処理するか否か ( 処理しないとき false を返すようにする )
	virtual bool requiresPre ( target_t ) const { return true; }
	virtual bool requiresEach( target_t ) const { return true; }
	virtual bool requiresPost( target_t ) const { return true; }
	
	// 処理 ( 実装 )
	virtual void procImplPre ( target_t ) { }
	virtual void procImplEach( target_t ) { }
	virtual void procImplPost( target_t ) { }
};

//##############################################################################
//                宣言部 : IFlatVisitor
//##############################################################################
//------------------------------------------------
// flat-visitor
// 
// @ 巡回(traverse)ではなく反復(iterate)。
// @ 最初に訪れたノードの子ノードでのみ処理をする。
// @	( ネスト 2 以下は requires* 時点で弾く )
//------------------------------------------------
template<class T>
class IFlatVisitor
	: public IVisitor<T>
{
public:
	IFlatVisitor()
		: mcntNest( 0 )
		, mbBlock( false )
	{ }
	virtual ~IFlatVisitor() { }
	
	//******************************************************
	//    インターフェース
	//******************************************************
public:
	virtual void visit( target_t tar )
	{
		if ( !mbBlock ) {
			mcntNest = 0;		// ネストを初期化
			mbBlock  = true;	// ブロック開始
				visit_impl( tar );
			mbBlock  = false;	// ブロック解除
			
		} else if ( isProcable() ) {
			mcntNest ++;
				visit_impl( tar );
			mcntNest --;
		}
		return;
	}
	
	// 処理 ( requires チェックのみ )
//	virtual void procPre ( target_t p ) { if ( requiresPre () ) procImplPre (p); }
//	virtual void procEach( target_t p ) { if ( requiresEach() ) procImplEach(p); }
//	virtual void procPost( target_t p ) { if ( requiresPost() ) procImplPost(p); }
	
protected:
	virtual void visit_impl( target_t pNode ) = 0;
	
	// 処理するか否か ( 処理しないとき false を返すようにする )
//	virtual bool requiresPre ( target_t ) const { return true; }
//	virtual bool requiresEach( target_t ) const { return true; }
//	virtual bool requiresPost( target_t ) const { return true; }
	
	// 処理 ( 実装 )	
//	virtual void procImplPre ( target_t ) { }
//	virtual void procImplEach( target_t ) { }
//	virtual void procImplPost( target_t ) { }
	
	// 処理できるかどうか
	virtual bool isProcable(void) const
	{
		return ( mbBlock && mcntNest != 0 );
	}
	
private:
	bool mbBlock;
	int  mcntNest;
	
};

#endif
