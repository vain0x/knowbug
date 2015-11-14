// visitor - データ文字列の生成者

#ifndef IG_CLASS_DATA_STRING_CREATOR_H
#define IG_CLASS_DATA_STRING_CREATOR_H

#include "Node.h"
#include "ITreeVisitor.h"

#include "module/CStrBuf.h"

namespace DataTree
{;

//------------------------------------------------
// データ文字列の生成者
//------------------------------------------------
class CDataStringCreator
	: public ITreeVisitor
{
public:
	CDataStringCreator();
	virtual ~CDataStringCreator() { }

	void setLenLimit(int lenLimit);
	string const& getString() const;

public:
	void visit0(ITree*) override;
	void visit(CLoopNode*) override;

	void visit(CNodeModule*) override;
	void visit(CNodeVarArray*) override;
	void visit(CNodeVarElem*) override;
	void visit(CNodeVarHolder*) override;

	void visit(CNodeModInst*) override;
	void visit(CNodeModInstNull*) override;
	void visit(CNodePrmStk*) override;

	void visit(CNodeLabel*) override;
	void visit(CNodeString*) override;
	void visit(CNodeDouble*) override;
	void visit(CNodeInt*) override;
	void visit(CNodeUnknown*) override;

private:
	void catStandardMultiArrayPre(char const* name, PVal* pval, size_t cntElems, size_t maxDim, size_t const* lengths);
	void catStandardMultiArrayEach(PVal* pval);
	void catStandardMultiArrayPost(PVal* pval);

protected:
	// テンプレートメソッド (一行か複数行かの切り替えになる)
	virtual void catNodeItemPre(char const* name);
	virtual void catNodeItemAnnotation(string const& name, string const& value);
	virtual void catNodeItemEach();
	virtual void catNodeItemPost();
	virtual void catLeafItem(char const* name);
	
protected:
	//*
	void cat(char const* s);
	void cat(string const& s) { cat(s.c_str()); }
	void catln(char const* s) { cat(s); catCrlf(); }
	void catln(string const& s) { cat(s); catCrlf(); }
	void catCrlf();
	void catDump( void const*, size_t );
	//*/
	
private:
	std::unique_ptr<CStrBuf> buf_;
};

// 複数行文字列を作成する場合
class CDataStringFullCreator
	: public CDataStringCreator
{
public:
	CDataStringFullCreator()
		: cntNest_(0)
	{ }

protected:
	void catNodeItemPre(char const* name) override;
	void catNodeItemEach() override { catCrlf(); catIndent(); }
	void catNodeItemAnnotation(string const& name, string const& value) override;
	void catNodeItemPost() override { decStrNest(); }
	void catLeafItem(char const* name) override;

private:
	void incStrNest() { ++cntNest_; }
	void decStrNest() { --cntNest_; }
	void catIndent() { cat(string('\t', cntNest_)); }

	int cntNest_;
};

// 一行文字列を作成する場合
class CDataStringLineCreator
	: public CDataStringCreator
{
public:
	CDataStringLineCreator() { }

protected:
	void catNodeItemPre(char const* name) override;
	void catNodeItemEach() override;
	void catNodeItemAnnotation(string const& name, string const& value) override { }
	void catNodeItemPost() override;
	void catLeafItem(char const* name) override;

};
}

#endif
