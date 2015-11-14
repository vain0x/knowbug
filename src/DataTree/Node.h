#ifndef IG_CLASS_DATA_TREE_NODE_H
#define IG_CLASS_DATA_TREE_NODE_H

#include "main.h"

#include <string>
#include <vector>
#include <memory>

#include "Node.dec.h"
#include "ITreeVisitor.h"
#include "ITreeObserver.h"

namespace DataTree
{

// 順番を変えてはいけない
struct TreeObservers
{
	ITreeObserver* appendObserver;
	ITreeObserver* removeObserver;
};
extern std::vector<TreeObservers> stt_observers;
extern void registerObserver(TreeObservers obs);

//##############################################################################
//                宣言部 : ITree
//##############################################################################
//------------------------------------------------
// データツリー
// 
// HSP上のデータをひとまとめに扱う構造。
// 基本的に mutable (const でない)。
// memo: 接頭辞 I を持つが interface ではなく抽象クラスでは……
//------------------------------------------------
class ITree
{
public:
	virtual ~ITree() { }

	// visitor にこのノードの実際の型を教える
	virtual void acceptVisitor(ITreeVisitor& visitor) = 0;

protected:
	template<typename TNode>
	void acceptVisitorTemplate(ITreeVisitor& visitor)
	{
		assert(dynamic_cast<TNode*>(this) != nullptr);
		visitor.visit(this_);
	}

public:
	// 現状に合わせて木構造を更新する
	// 子ノード child が nullptr ではない場合、そのノードが更新後も子ノードであるなら true を、消滅したなら false を返す。
	// nullptr である場合、各子ノードに更新を要求し、true を返す。
	virtual bool updateState(tree_t childOrNull) = 0;

	void updateState() { updateState(nullptr); }

	// 更新状態
public:
	enum class UpdatedState { None = 0, Shallow = 1, Deep = 2 };
	void setUpdatedState(UpdatedState s) { updatedState_ = s; }
	UpdatedState getUpdatedState() const { return updatedState_; }
private:
	UpdatedState updatedState_;

};

//------------------------------------------------
// 抽象コンテナノード
// 
// 実際には IMonoNode か IPolyNode である。
// 名前を持ち、動的変化する。
// 子ノードのポインタの所有権を持つ。
//------------------------------------------------
class INodeContainer
	: public ITree
{
protected:
	typedef std::vector<tree_t> children_t;

public:
	INodeContainer(tree_t parent, string const& name)
		: ITree(), parent_(parent), name_(name)
	{ }
	
	// 親
public:
	tree_t getParent() const { return parent_; }

private:
	tree_t parent_;

	// 名前
public:
	string const& getName() const { return name_; }
	
protected:
	void rename(string const& name) { name_ = std::move(name); }
	
private:
	string name_;

	// 動的変化
	/*
public:
	enum class CurrentStateType {
		NoChange = 0,
		Removed,	// このノードが既に消失している
		Remaked,	// このノードが(存在はしているが)作り直されたかどうか
		Extended,	// このノードに子ノードが追加されたかどうか (削除はされてない)
		Modified,	// このノードの値が変更された
	};

	struct CurrentState {
		virtual ~CurrentState() { }
	};
protected:
	typedef std::shared_ptr<CurrentState> change_t;

public:
	// 現在の状態を調査する
	// 具体的な情報は各ノードごとに ChangeStruct を継承したものを返却して伝達せよ。
	// TODO: 純粋仮想( = 0 )にする
	// change より modify の方がらしそう
	virtual change_t inspectCurrentState() const { return nullptr; }
	//*/
};

//*
//------------------------------------------------
// 単体コンテナノード
// 
// @ データ文字列では、「... =」を構築する。
//------------------------------------------------
class IMonoNode
	: public INodeContainer
{
public:
	IMonoNode(tree_t parent, string const& name, tree_t child)
		: INodeContainer(parent, name)
		, child_(child)
	{ }

	virtual ~IMonoNode() {
		removeChild();
	}

public:
	tree_t getChild() const { return child_; }

protected:
	void setChild(tree_t child);

private:
	void removeChild();

private:
	tree_t child_;
};
//*/

//------------------------------------------------
// 複数コンテナノード
// 
// @ 複数の(0個以上の)ノードを持つノード。
// @ データ文字列では、「*: ...」を構築する。
// @ex: array引数
//------------------------------------------------
class IPolyNode
	: public INodeContainer
{
public:
	IPolyNode(tree_t parent, string const& name)
		: INodeContainer(parent, name)
		, children_()
	{ }

	virtual ~IPolyNode() {
		removeChildAll();
	}
	
	children_t const& getChildren() { return children_; }
protected:
	tree_t addChild(tree_t child);
	tree_t replaceChild(children_t::iterator& pos, tree_t child);

private:
	void removeChild(children_t::iterator& pos);
	void removeChildAll() {
		for ( auto iter = children_.begin(); iter != getChildren().end(); ++iter ) {
			removeChild(iter);
		}
		children_.clear();
	}
	
private:
	children_t children_;
};

//------------------------------------------------
// ループノード
// 
// @ 祖先ノードと同一のノードを指し示す。
//------------------------------------------------
class CLoopNode
	: public IMonoNode
{
public:
	CLoopNode(tree_t parent, tree_t ancestor)
		: IMonoNode(parent, "(loop)", ancestor)
	{ }

	void acceptVisitor(ITreeVisitor& visitor) override {
		visitor.visit(this);
	}
};

//------------------------------------------------
// リーフノード
// 
// データ文字列では、「=」の右辺を構築する。
//------------------------------------------------
class ILeaf
	: public ITree
{
public:
	ILeaf() { }
};

//##############################################################################
//                宣言部 : 具象ノード
//##############################################################################

//**********************************************************
//        領域
//**********************************************************
//------------------------------------------------
// モジュール
//------------------------------------------------
class CNodeModule
	: public IPolyNode
{
public:
	CNodeModule(tree_t parent, string const& name);

	void acceptVisitor(ITreeVisitor& visitor) override {
		acceptVisitorTemplate<CNodeModule>(visitor);
	}

	bool updateState(tree_t childOrNull) override {
		if ( !childOrNull ) {
			for ( auto child : getChildren() ) {
				child->updateState();
			}
		}
		return true;
	}

protected:
	void addVar(const char* fullName);
private:
	void addVarUnscoped(char const* fullName, string const& rawName);

	CNodeModule& findNestedModule(char const* scopeResolt);
	CNodeModule* addModule(string const& rawName);

	virtual bool contains(char const* name) const;
	virtual string unscope(string const& scopedName) const;
};

//------------------------------------------------
// グローバル領域
// 
// @ root 要素。
// @ シングルトン。
// @ 名前修飾を使用しない。
//------------------------------------------------
class CNodeGlobal
	: public CNodeModule
{
public:
	static CNodeGlobal& getInstance() {
		return *(instance ? instance : (instance = new CNodeGlobal()));
	}

private:
	static CNodeGlobal* instance;
	CNodeGlobal();

	// ノードとしての振る舞いは変わらないので acceptVisitor を再定義しない。
	// void acceptVisitor(ITreeVisitor&) override;

private:
	bool contains(char const* name) const override { return true; }
	string unscope(string const& scopedName) const override;
};

//**********************************************************
//        変数
//**********************************************************

// 配列、要素の上位に(抽象)変数クラスを挟む？
// 変数は値を展開する・しないにかかわらず配列か否かの判断をすべきか？
// 配列／要素、にならべて、「未展開変数」クラスを作るとか。

// inspectCurrentState 自体が値の要求として子ノードの展開を求めているのでは？
// というか change の一種として「子ノードを展開せよ」があるのか
// 展開しているかどうかのフラグを持つべきか？

// inspectCurrentState 

//------------------------------------------------
// 変数
//
// (未使用)
// @ CNodeVarArray か CNodeVarElem を持つ。
//------------------------------------------------
class CNodeVarHolder
	: public IMonoNode
{
public:
	CNodeVarHolder(tree_t parent, string const& name, PVal* pval)
		: IMonoNode(parent, name, nullptr)
		, pval_(pval)
	{ }

	void acceptVisitor(ITreeVisitor& visitor) override {
		acceptVisitorTemplate<CNodeVarHolder>(visitor);
	}
	
	bool updateState(tree_t childOrNull) override;
private:
	PVal* pval_;
};

//------------------------------------------------
// 配列
// 
// @ 一要素の変数は含まない。
// @ 要素の数だけ変数要素を含むコンテナとみなす。
// TODO: 静的変数ならとにかく、ローカル変数(やメンバ変数)などは存在しているかすらも定かでない。
//	生成直後以外に pval_ に触れてはならない。
//------------------------------------------------
class CNodeVarArray
	: public IPolyNode
{
public:
	CNodeVarArray(tree_t parent, string const& name, PVal* pval);

	void acceptVisitor(ITreeVisitor& visitor) override {
		acceptVisitorTemplate<CNodeVarArray>(visitor);
	}

	/*
	返しうる情報
		・この変数は既に存在していない
			親(prmstk、または外部プラグイン定義のオブジェクト)が死んでいたり変更されている場合
			静的変数ならありえない。
			親を辿るための何かが必要
		・モードが変化した
		・変数型が変化した
			変数自体別物なので、子ノードを総取っ替えする
			そういえばダングリングリンクになっているクローン変数があるとデバッガごと落ちるのでは
		・要素数、が変化した
			次元が下がったか、要素数が減った場合は、
				変数自体が取り替えられているので型が変化したのと同じ扱いでいい
				→拡張型によっては同じ変数の要素数が減少しうるかもしれない
				そうか、変数の扱いってその型に対応する処理を行うべきなのか
			次元が上がった場合、
				子ノードの名前や順番が変わる ((a, b) が (a, b, c) になったり)
			次元が同じで、単に配列が伸びた場合には、
				伸びた分を追加して、元々あった要素は再計算する。
				再計算すべきである、という情報を伝達する必要がある。
		・要素の中身が変化した
			要素の値を再計算する
	*/

	bool updateState(tree_t childOrNull);

	// 持ちデータ
	PVal* getPVal() const { return pval_; }
	size_t cntElems() const { return cntElems_; }
	size_t getMaxDim() const { return maxDim_; }
	size_t const* getLengths() const { return length_; }

private:
	void initialize();
	void addElem(size_t aptr, size_t const* idxes);
	
private:
	PVal* pval_;

	size_t cntElems_;
	size_t maxDim_;
	size_t length_[hpimod::ArrayDimMax];

	vartype_t vt_;
	varmode_t mode_;
};

//------------------------------------------------
// 変数要素
// 
// @ 要素番号が決定されているもの。
// @ 配列の要素か、var 引数。
//------------------------------------------------
class CNodeVarElem
	: public IMonoNode
{
public:
	CNodeVarElem(tree_t parent, string name, PVal* pval, APTR aptr);
	
	void acceptVisitor(ITreeVisitor& visitor) override { acceptVisitorTemplate<CNodeVarElem>(visitor); }
	bool updateState(tree_t childOrNull) override;

private:
	void reset();

private:
	PVal* pval_;
	APTR  aptr_;
};

//**********************************************************
//        型ごとのデータ
//**********************************************************
//------------------------------------------------
// 単純値
//------------------------------------------------
template<class TSelf, class TVal, vartype_t TVartype, class TNode>
class CNodeValue
	: public TNode
{
public:
	typedef TVal value_type;

public:
	CNodeValue(TVal const& val)
		: val_(val)
	{ }

	template<class = std::enable_if_t<std::is_convertible<TNode*, INodeContainer*>::value>>
	CNodeValue(tree_t parent, TVal const& val)
		: TNode(parent), val_(val)
	{ }

public:
	void acceptVisitor(ITreeVisitor& visitor) override {
		acceptVisitorTemplate<TSelf>(visitor);
	}

	virtual vartype_t getVartype() const { return TVartype; }
	TVal const& getValue() const { return val_; }

private:
	TVal const val_;
};

//------------------------------------------------
// label 型
//------------------------------------------------
class CNodeLabel
	: public CNodeValue<CNodeLabel, label_t, HSPVAR_FLAG_LABEL, ILeaf>
{
public:
	CNodeLabel(label_t val) : CNodeValue(val) { }
};

//------------------------------------------------
// str 型
//------------------------------------------------
class CNodeString
	: public CNodeValue<CNodeString, char const*, HSPVAR_FLAG_STR, ILeaf>
{
public:
	CNodeString(char const* val) : CNodeValue(val) { }
};

//------------------------------------------------
// double 型
//------------------------------------------------
class CNodeDouble
	: public CNodeValue<CNodeDouble, double, HSPVAR_FLAG_DOUBLE, ILeaf>
{
public:
	CNodeDouble(double val) : CNodeValue(val) { }
};

//------------------------------------------------
// int 型
//------------------------------------------------
class CNodeInt
	: public CNodeValue<CNodeInt, int, HSPVAR_FLAG_INT, ILeaf>
{
public:
	CNodeInt(int val) : CNodeValue(val) { }
};

//------------------------------------------------
// struct 型
//------------------------------------------------
class CNodeModInst
	: public CNodeValue<CNodeModInst, FlexValue const*, HSPVAR_FLAG_STRUCT, IMonoNode>
{
public:
	CNodeModInst(tree_t parent, FlexValue const* p);
};

// nullmod
// @ singleton
// (Node の型自体を分けておかないと Visitor 側で場合分けを procPre, procPost の2回行うことになってしまう)
class CNodeModInstNull
	: public CNodeValue<CNodeModInstNull, FlexValue const*, HSPVAR_FLAG_STRUCT, ILeaf>
{
public:
	static CNodeModInstNull& getInstance() { return const_cast<CNodeModInstNull&>(instance); }
private:
	CNodeModInstNull()
		: CNodeValue(nullptr)
	{ }

	static CNodeModInstNull const instance;
};

//------------------------------------------------
// 不明な型
//------------------------------------------------
class CNodeUnknown
	: public CNodeValue<CNodeUnknown, void const*, 0, ILeaf>
{
public:
	CNodeUnknown(void const* p, vartype_t vt)
		: CNodeValue(p), type_(vt)
	{ }

	vartype_t getVartype() const override { return type_; }
	
private:
	vartype_t type_;
};

//**********************************************************
//        その他のデータ
//**********************************************************
//------------------------------------------------
// prmstk
//------------------------------------------------
class CNodePrmStk
	: public IPolyNode
{
public:
	CNodePrmStk(tree_t parent, string name, void* prmstk, stdat_t stdat);
	CNodePrmStk(tree_t parent, string name, void* prmstk, stprm_t stprm);

public:
	void acceptVisitor(ITreeVisitor& visitor) override {
		acceptVisitorTemplate<CNodePrmStk>(visitor);
	}

private:
	void initialize();
	void add( size_t idx, void* member, stprm_t stprm );
	
private:
	void* prmstk_;
	stdat_t stdat_;
	stprm_t stprm_;
};

/*
//------------------------------------------------
// 単純な文字列
//------------------------------------------------
class CNodeSimpleStr
	: public ILeaf
{
public:
	CNodeSimpleStr(string s) : str_(s) { }
	
	string getDataString() const { return str_; }
	
private:
	string str_;
};
//*/

}

#endif
