// visitor - データ文字列の生成者

// アノテーション (#length = ... など) は CNodeAnnotation とするべきな気もするが、単にメモリの無駄遣いなだけな気がする

#include "main.h"
#include "DebugInfo.h"

#include "Node.h"
#include "CDataStringCreator.h"

#include "module/CStrBuf.h"
#include "module/ptr_cast.h"
#include "module/strf.h"
//#include "module/debug.h"

#ifdef with_Assoc
# include "D:/Docs/prg/cpp/MakeHPI/hpimod/var_assoc/for_knowbug.var_assoc.h"	// あまりにも遠いのでフルパス
#endif
#ifdef with_Vector
# include "D:/Docs/prg/cpp/MakeHPI/hpimod/var_vector/for_knowbug.var_vector.h"	// あまりにも遠いのでフルパス
#endif
#ifdef with_Array
# include "D:/Docs/prg/cpp/MakeHPI/var_array/src/for_knowbug.var_array.h"	// あまりにも遠いのでフルパス
#endif

#include "with_Script.h"
#include "with_ModPtr.h"

namespace DataTree
{
;

//------------------------------------------------
// 構築
//------------------------------------------------
CDataStringCreator::CDataStringCreator()
	: buf_(new CStrBuf)
{ }

void CDataStringCreator::setLenLimit(int len)
{
	buf_->setLenLimit(len);
	return;
}

string const& CDataStringCreator::getString() const
{
	return buf_->get();
}

//------------------------------------------------
// 訪問
//------------------------------------------------
void CDataStringCreator::visit0(ITree* t)
{
	dbgout("visit to %X", t);
	t->acceptVisitor(*this);
	return;
}

//------------------------------------------------
// モジュール
//------------------------------------------------
void CDataStringCreator::visit(CNodeModule* p)
{
	catNodeItemPre(p->getName().c_str());

	for ( auto child : p->getChildren() ) {
		catNodeItemEach();
		visit0(child);
	}

	catNodeItemPost();
	return;
}

//------------------------------------------------
// 変数
//------------------------------------------------
void CDataStringCreator::visit(CNodeVarArray* p)
{
	PVal* const pval = p->getPVal();

	switch (pval->flag) {
		case HSPVAR_FLAG_LABEL:
		case HSPVAR_FLAG_STR:
		case HSPVAR_FLAG_DOUBLE:
		case HSPVAR_FLAG_INT:
		case HSPVAR_FLAG_STRUCT:
		{
			catNodeItemPre(p->getName().c_str());

			auto const vartypeName = hpimod::getHvp(pval->flag)->vartype_name;

			// 一次元配列
			if ( pval->len[2] <= 0 ) {
				string const format = makeArrayIndexString(1, p->getLengths());
				catNodeItemAnnotation("vartype",
					strf("%s %s", vartypeName, format.c_str()));

			// 多次元配列
			} else {
				string const format = makeArrayIndexString(p->getMaxDim(), p->getLengths());
				catNodeItemAnnotation("vartype",
					strf("%s %s (%d in total)", vartypeName, format.c_str(), p->cntElems()));
			}
			
			for ( auto child : p->getChildren() ) {
				catNodeItemEach();
				visit0(child);
			}

			catNodeItemPost();
			break;
		}
		// TODO: 拡張型の場合は外部から与えられた関数に丸投げしたい
		default:
			catLeafItem(p->getName().c_str());
			cat("unknown");
			break;
	}
}

//------------------------------------------------
// 変数要素
//------------------------------------------------
void CDataStringCreator::visit(CNodeVarElem* p)
{
	catLeafItem(p->getName().c_str());
	visit0(p->getChild());
}

//------------------------------------------------
// label 値
//------------------------------------------------
void CDataStringCreator::visit(CNodeLabel* p)
{
	auto const lb = p->getValue();
	int const idx = hpimod::findOTIndex(lb);
	auto const name =
		(idx >= 0 ? g_dbginfo->ax->getLabelName(idx) : nullptr);
	cat( (name ? strf("*%s", name) : strf("*label (0x%08X)", address_cast(lb))) );
	return;
}

//------------------------------------------------
// str 値
//------------------------------------------------
void CDataStringCreator::visit(CNodeString* p)
{
	cat(p->getValue());
}

//------------------------------------------------
// double 値
//------------------------------------------------
void CDataStringCreator::visit(CNodeDouble* p)
{
	cat(strf("%.16f", p->getValue()));
}

//------------------------------------------------
// int 値
//------------------------------------------------
void CDataStringCreator::visit(CNodeInt* p)
{
	int const val = p->getValue();

#ifdef with_ModPtr
	// TODO: node を生成する段階で、int の代わりに ModInst を生成する
	// "mp#%d" を出力するためには、ModInst を持つ CNodeModPtr (<: IMonoNode) を生成する

	if (ModPtr::isValid(val)) {
		CNodeModInst node(ModPtr::getValue(val));
		cat(strf("mp#%d", ModPtr::getIdx(val)));
		visit0(&node);
		return;
	}
#endif

	cat(strf("%-10d (0x%08X)", val, val));
	return;
}

//------------------------------------------------
// struct 値
// 
// @ ... = (struct: 型名&):
// @	<prmstk>
//------------------------------------------------
void CDataStringCreator::visit(CNodeModInst* p)
{
	auto const fv = p->getValue();

	string const name = strf("(struct: %s%s)",
		hpimod::STRUCTDAT_getName(hpimod::FlexValue_getStDat(fv)),
		((fv->type == FLEXVAL_TYPE_CLONE) ? "&" : "")
	);

	catNodeItemPre(name.c_str());

	visit0(p->getChild());	// prmstk

	catNodeItemPost();
	return;
}

void CDataStringCreator::visit(CNodeModInstNull* p)
{
	cat("(struct: null)");
}

//------------------------------------------------
// prmstk
// 
// struct 型変数にのみ格納される。
// これが呼ばれる時点で「*: ...」の字下げは行われている。
//------------------------------------------------
void CDataStringCreator::visit(CNodePrmStk* p)
{
	for ( auto const child : p->getChildren() ) {
		catNodeItemEach();
		visit0(child);
	}
	return;
}

//**********************************************************
//    文字列の追加
//**********************************************************
//------------------------------------------------
// 単純連結
//------------------------------------------------
void CDataStringCreator::cat(char const* s)
{
	buf_->cat(s);
}

//------------------------------------------------
// 改行の連結
//------------------------------------------------
void CDataStringCreator::catCrlf()
{
	buf_->catCrlf();
}

//------------------------------------------------
// バッファを16進数でメモリダンプして連結
//------------------------------------------------
void CDataStringCreator::catDump(void const* pBuf, size_t bufsize)
{
	static const size_t stc_maxsize = 0x10000;
	size_t size = bufsize;

	if ( size > stc_maxsize ) {
		cat(strf("全%dバイトの内、%dバイトのみをダンプします。", bufsize, stc_maxsize));
		size = stc_maxsize;
	}

	catln("dump  0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F");
	catln("----------------------------------------------------");

	buf_->catDump(pBuf, size, 0x10);

	catCrlf();
	catln(strf("バッファサイズ：%d (bytes)", bufsize));
	return;
}

//**********************************************************
// 複数行文字列生成者
//**********************************************************
// コンテナノードの文字列の追加準備
void CDataStringFullCreator::catNodeItemPre(char const* name)
{
	dbgout("poly-container %s", name);
	cat(name);
	catln(":");
	incStrNest();
	return;
}

void CDataStringFullCreator::catNodeItemAnnotation(string const& name, string const& value)
{
	catNodeItemEach();

	cat(name);
	cat(" = ");
	cat(value);
	return;
}

// リーフノードの文字列の追加準備
void CDataStringFullCreator::catLeafItem(char const* name)
{
	dbgout("mono-container %s", name);
	cat(name);
	cat(" = ");
	return;
}

//**********************************************************
// 一行文字列生成者
//**********************************************************
// コンテナノードの文字列の追加準備
void CDataStringLineCreator::catNodeItemPre(char const* name)
{
	dbgout("poly-container %s", name);
	cat("[");
	return;
}

void CDataStringLineCreator::catNodeItemEach()
{
	cat(", ");
	return;
}

void CDataStringLineCreator::catNodeItemPost()
{
	cat("]");
	return;
}

// リーフノードの文字列の追加準備
void CDataStringLineCreator::catLeafItem(char const* name)
{
	dbgout("mono-container %s", name);
	//
	return;
}

}
