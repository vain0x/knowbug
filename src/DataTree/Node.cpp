// 動的ツリー

#include "main.h"
#include "DebugInfo.h"

#include "Node.h"

//#include "CNodeIterator.h"
#include "module/ptr_cast.h"
#include "module/strf.h"
#include "module/supio/supio.h"

#ifdef clhsp
# include "chruntime/mod_vector.h"
#endif

namespace DataTree
{

// 既知とする変数型
static bool Vartype_isKnown(vartype_t vt) { return HSPVAR_FLAG_LABEL <= vt && vt <= HSPVAR_FLAG_STRUCT; }

// 観測者
std::vector<TreeObservers> stt_observers;
void registerObserver(TreeObservers r) { stt_observers.push_back(r); }
static std::vector<TreeObservers>& getObservers() { return stt_observers; }

// 木の静的変数
CNodeGlobal* instance = nullptr;
CNodeModInstNull const CNodeModInstNull::instance;

//##############################################################################
//                非メンバ関数
//##############################################################################
//------------------------------------------------
// PVal から変数ノードを生成する
// 
// @ struct までは認知するが、他は unknown とする。
// @ TODO: 拡張型では変数要素か否かの判定は異なる可能性がある (例：vector_k)
//------------------------------------------------
static tree_t new_nodeVar(tree_t parent, string const& name, PVal* pval)
{
	if (//(pval->support & (HSPVAR_SUPPORT_FIXEDARRAY | HSPVAR_SUPPORT_FLEXARRAY))
		Vartype_isKnown(pval->flag)
		&& !(pval->len[1] == 1 && pval->len[2] <= 0)) {
		return tree_t(new CNodeVarArray(parent, name, pval));

	} else {
		return tree_t(new CNodeVarElem(parent, name, pval, 0));
	}
}

//------------------------------------------------
// PVal から変数ノードを生成する
//------------------------------------------------
static tree_t new_nodeVar(tree_t parent, string const& name, PVal* pval, APTR aptr)
{
	return tree_t( new CNodeVarElem(parent, name, pval, aptr) );
}

//------------------------------------------------
// ノード生成 (struct値)
//------------------------------------------------
static tree_t new_nodeValue_struct(tree_t parent, FlexValue const* fv)
{
	assert(fv != nullptr); if (!fv) return nullptr;

	if (!fv->ptr || fv->type == FLEXVAL_TYPE_NONE) {
		return &CNodeModInstNull::getInstance();
	} else {
		return new CNodeModInst(parent, fv);
	}
}

//------------------------------------------------
// ノード生成 (from: ポインタと型)
// 
// 拡張型については HPI に任せる (そのためにこういうノード生成系関数は公開しておく)。
//------------------------------------------------
static tree_t new_nodeValue_impl( tree_t parent, void const* p, vartype_t vt )
{
	switch ( vt ) {
		case HSPVAR_FLAG_LABEL:  return new CNodeLabel ( *ptr_cast<label_t const*>(p) );
		case HSPVAR_FLAG_STR:    return new CNodeString(  ptr_cast<char const*>(p) );
		case HSPVAR_FLAG_DOUBLE: return new CNodeDouble( *ptr_cast<double const*>(p) );
		case HSPVAR_FLAG_INT:    return new CNodeInt   ( *ptr_cast<int const*>(p) );
		case HSPVAR_FLAG_STRUCT: return new_nodeValue_struct(parent, ptr_cast<ModInst const*>(p));
	//	case HSPVAR_FLAG_COMOBJ:
	//	case HSPVAR_FLAG_VARIANT:
		default:
			return new CNodeUnknown(parent, p, vt );
	}
	
//	return NULL;
}

static tree_t new_nodeValue(tree_t parent, void const* ptr, vartype_t vt )
{
	return new_nodeValue_impl(parent, ptr, vt );
}

//##############################################################################
//                定義部 : ITree とその派生クラス
//##############################################################################

//**********************************************************
//        ノード ( 単体コンテナ )
//**********************************************************

void IMonoNode::setChild(tree_t child)
{
	removeChild();
	for ( auto& r : getObservers() ) {
		r.appendObserver->visit0(child);
	}
	child_ = child;
	return;
}

void IMonoNode::removeChild() {
	if ( !child_ ) return;
	for ( auto &r : getObservers() ) {
		r.removeObserver->visit0(child_);
	}
	delete child_; child_ = nullptr;
	return;
}

//**********************************************************
//        ノード ( 複数コンテナ )
//**********************************************************

//------------------------------------------------
// 子ノードの追加
//------------------------------------------------
tree_t IPolyNode::addChild( tree_t child )
{
	for ( auto& r : getObservers() ) {
		r.appendObserver->visit0(child);
	}
	children_.push_back( child );
	return child;
}

tree_t IPolyNode::replaceChild(children_t::iterator& pos, tree_t child)
{
	removeChild(pos);
	*pos = child;
	return;
}

void IPolyNode::removeChild(children_t::iterator& pos)
{
	auto& child = *pos;
	for ( auto &r : getObservers() ) {
		r.removeObserver->visit0(child);
	}
	delete child; child = nullptr;
	return;
}

//**********************************************************
//        領域
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CNodeModule::CNodeModule(tree_t parent, string const& name)
	: IPolyNode( parent, name )
{ }

//------------------------------------------------
// このモジュールに属すか？
// 
// @ global では override される。
//------------------------------------------------
bool CNodeModule::contains( char const* name) const
{
	size_t const lenModule = getName().length();
	size_t const lenName = strlen(name);
	
	// name の末尾が "@モジュール名" であればよい
	return ( lenName > lenModule + 1 )
		&&  name[lenName - lenModule - 1] == '@'
		&& &name[lenName - lenModule] == getName();
}

//------------------------------------------------
// 名前解決を取り除く
//------------------------------------------------
string CNodeModule::unscope(string const& scopedName) const
{
	assert(contains(scopedName.c_str()));
//	scopedName.erase(scopedName.length - (1 + getName().length()));
	return scopedName.substr(0, scopedName.length() - (1 + getName().length()));
}

//------------------------------------------------
// 静的変数を追加する
//
// @ スコープ解決がついているなら、子モジュールを挿入して、その下に追加する。
// @ この関数自体は高々1回の再帰で完了する。ほとんどは findModule に押し付けた。
//------------------------------------------------
void CNodeModule::addVar(char const* name)
{
	char const* const scopeResoltAll = std::strchr(name, '@');

	if ( scopeResoltAll ) {
		auto& mod = findNestedModule(scopeResoltAll);
		mod.addVarUnscoped(name, string(name, scopeResoltAll));

	} else {
		addVarUnscoped(name, string(name));
	}
}

void CNodeModule::addVarUnscoped(char const* fullName, string const& rawName)
{
	assert(rawName.find('@') == string::npos);

	int const iVar = exinfo->HspFunc_seekvar(fullName);
	assert(0 <= iVar && iVar < ctx->hsphed->max_val);

	addChild(new_nodeVar(this, rawName, hpimod::getPVal(iVar)));
	return;
}

//------------------------------------------------
// 入れ子になったモジュールを求める
// 
// @prm scopeResoltion:
//	求めるモジュールを指し示す完全修飾
//	例えば、静的変数「var@inner@outer」があるとき、
//	inner モジュールを指し示す完全修飾は「@inner@outer」。
//	グローバル領域を指し示す完全修飾は空文字列だが、与えられることはない。
// 検索部分を1回しか書かないために、経路ではなく式で場合分けをしていて、ややこしい。
//------------------------------------------------
CNodeModule& CNodeModule::findNestedModule(char const* scopeResolt)
{
//	if (scopeResolt[0] == '\0') return *CNodeModule::getInstance();
	assert(scopeResolt[0] == '@');
	
	// 次の階層のモジュールを探す
	// 例1. scopeResolt = "@inner@middle@outer" なら "@outer"
	// 例2. scopeResolt = "@inner" なら nullptr
	char const* const nextScopeResolt = std::strrchr(scopeResolt + 1, '@');

	// このモジュールが含むモジュールの非修飾名
	// 例1. "outer"
	// 例2. "inner"
	string submodName(
		(nextScopeResolt)
			? nextScopeResolt + 1
			: scopeResolt + 1
	);
	assert(submodName.find('@') == string::npos);

	// モジュール名で検索
	CNodeModule* submod = nullptr;
	for ( auto const it : getChildren() ) {
		submod = dynamic_cast<CNodeModule*>(it);
		if ( submod && submod->getName() == submodName ) break;
	}

	// (なければ) モジュール名リストに追加する
	if ( !submod ) {
		submod = addModule(std::move(submodName));
	}

	// 続きがあれば、さらに探索を続ける
	if ( nextScopeResolt ) {
		string const remainScopeResolt(scopeResolt, nextScopeResolt);
		return submod->findNestedModule(remainScopeResolt.c_str());

	// なければ、今挿入・発見したモジュールが求めるモジュールである
	} else {
		return *submod;
	}
}

CNodeModule* CNodeModule::addModule(string const& rawName)
{
	auto const mod = new CNodeModule(this, rawName);
	addChild(mod);
	return mod;
}

//------------------------------------------------
//
//------------------------------------------------


//------------------------------------------------
// グローバル領域
//------------------------------------------------
CNodeGlobal::CNodeGlobal()
	: CNodeModule(nullptr, "")
{
	// cf. getSttVarTree
	// TODO: 静的変数をなめてグローバル領域と変数ノードを生成する

	// すべての静的変数の名前のリストを要求する
	char* const p = g_dbginfo->debug->get_varinf(nullptr, 0xFF);
	
	char name[0x100];
	strsp_ini();
	for ( ;; ) {
		int chk = strsp_get(p, name, 0, 0x100 - 1);
		if ( chk == 0 ) break;

		addVar(name);
	}

	g_dbginfo->debug->dbg_close(p);
	return;
}

// 実質 static
string CNodeGlobal::unscope(string const& scopedName) const
{
	return ( scopedName.back() == '@' )
		? scopedName.substr(0, scopedName.length() - 1)
		: scopedName;
}

//**********************************************************
//        配列変数
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CNodeVarArray::CNodeVarArray(tree_t parent, string const& name, PVal* pval)
	: IPolyNode(parent, name)
	, pval_(pval)
{
	assert(!(pval_->len[1] == 1 && pval_->len[2] <= 0));
	initialize();
}

//------------------------------------------------
// 前処理
//------------------------------------------------
void CNodeVarArray::initialize()
{
	vt_ = pval_->flag;
	mode_ = pval_->mode;
	
	// 全要素数と最大次元を調べる
	for ( int i = 0; i < hpimod::ArrayDimMax; ++ i ) {
		length_[i] = pval_->len[i + 1];
		
		if (length_[i] <= 0) break;
		if ( i == 0 ) cntElems_ = 1;
		cntElems_ *= length_[i];
		maxDim_   ++;
	}
	
	for ( size_t i = 0; i < cntElems_; ++ i ) {
		// aptr を分解して添字を求める
		size_t aptr = i;
		size_t idx[hpimod::ArrayDimMax] = { 0 };
		for ( size_t idxDim = 0; idxDim < maxDim_; ++ idxDim ) {
			idx[idxDim] = aptr % length_[idxDim];
			aptr        = aptr / length_[idxDim];
		}
		
		// 要素を追加 (便宜上、単体変数として追加する)
		addElem(i, idx);
	}
	return;
}

void CNodeVarArray::addElem(size_t aptr, size_t const* idxes)
{
	addChild(new CNodeVarElem(this, makeArrayIndexString(maxDim_, idxes), pval_, aptr));
}

bool CNodeVarArray::updateState(tree_t childOrNull)
{
	if ( getParent()->updateState(this) ) {
		auto newCntElems = hpimod::PVal_cntElems(pval_);

		// 一旦初期化されたと思われる場合は、再構築する
		if ( mode_ != pval_->mode || vt_ != pval_->flag
			// 要素数が減った
			|| (hpimod::PVal_cntElems(pval_) < cntElems_)
			// 2次元以上になっていてかつ要素数が変化した (TODO: 自動拡張の場合は再初期化ではなく追加処理を行うべき、めんどくさいけど)
			|| (pval_->len[2] > 0 && newCntElems != cntElems_)
			) {
			initialize();

		// 各要素を再計算し、増えた分を追加する
		} else {
			assert(maxDim_ == 1 && newCntElems >= cntElems_);

			if ( !childOrNull ) {
				for ( auto& child : getChildren() ) {
					child->updateState();
				}
				for ( size_t i = cntElems_; i < newCntElems; ++i ) {
					addElem(i, &i);
				}
			}
			return true;
		}
	}
	return false;
}

//**********************************************************
//        単体変数
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CNodeVarElem::CNodeVarElem(tree_t parent, string name, PVal* pval, APTR aptr)
	: IMonoNode(parent, name, nullptr )
	, pval_( pval )
	, aptr_( (aptr < 0) ? 0 : aptr )
{
	reset();
	return;
}

void CNodeVarElem::reset()
{
	auto const hvp = hpimod::getHvp(pval_->flag);
	pval_->offset = aptr_;
	setChild(new_nodeValue(this, reinterpret_cast<void const*>(hvp->GetPtr(pval_)), pval_->flag));
	return;
}

bool CNodeVarElem::updateState(tree_t childOrNull)
{
	if ( getParent()->updateState(this) ) {
		reset();
		return true;
	}
	return false;
}

//**********************************************************
//        prmstk
//**********************************************************
//------------------------------------------------
// 構築
//------------------------------------------------
CNodePrmStk::CNodePrmStk(tree_t parent, string name, void* prmstk, stprm_t stprm)
	: IPolyNode( parent, name )
	, prmstk_( prmstk )
	, stprm_( stprm )
{
	stdat_ = hpimod::STRUCTPRM_getStDat(stprm_);
	initialize();
	return;
}

CNodePrmStk::CNodePrmStk(tree_t parent, string name, void* prmstk, stdat_t stdat)
	: IPolyNode( parent, name )
	, prmstk_( prmstk )
	, stdat_( stdat )
{
	stprm_ = hpimod::STRUCTDAT_getStPrm(stdat_);
	initialize();
	return;
}

// 前処理
void CNodePrmStk::initialize()
{
//	if ( getName() == "" ) rename( strf("prmstk(%s)", hpimod::STRUCTDAT_getName(stdat_)) );
	
	size_t idx = 0;
	std::for_each(stprm_, stprm_ + stdat_->prmmax, [this, &idx](STRUCTPRM const prm) {
		stprm_t const p = &prm;
		add(idx, ptr_cast<char*>(prmstk_) + p->offset, p);
		
		// structtag => 引数ではないので、数えない
		if ( p->mptype != MPTYPE_STRUCTTAG ) {
			++ idx;
		}
	});
	return;
}

//------------------------------------------------
// メンバの追加
//------------------------------------------------
void CNodePrmStk::add( size_t idx, void* member, stprm_t stprm )
{
	string name;
	{
		bool bNamed = false;

		int const subid = hpimod::findStPrmIndex(stprm);
		if (subid >= 0) {
			auto const pName = g_dbginfo->ax->getPrmName(subid);
			if (pName) {
				name = pName; bNamed = true;

				// スコープ解決を取り除く
				int const iScope = name.find('@');
				if (iScope != std::string::npos) {
					name = name.substr(0, iScope);
				}
			}
		}
		if (!bNamed) name = makeArrayIndexString(1, &idx);	// 仮の名称
	}
	
	switch ( stprm->mptype ) {
		// 変数 (PVal*)
	//	case MPTYPE_VAR:
		case MPTYPE_SINGLEVAR:
		{
			auto const v = ptr_cast<MPVarData*>(member);
			addChild(new_nodeVar(this, name, v->pval, v->aptr));
			break;
		}
		case MPTYPE_ARRAYVAR:
		{
			auto const v = ptr_cast<MPVarData*>(member);
			addChild( new_nodeVar(this, name, v->pval) );
			break;
		}
		// 変数 (PVal)
		case MPTYPE_LOCALVAR:
			addChild( new_nodeVar(this, name, ptr_cast<PVal*>(member)) );
			break;

		// thismod (MPModInst)
		case MPTYPE_MODULEVAR:
		case MPTYPE_IMODULEVAR:
		case MPTYPE_TMODULEVAR:
		{
			auto const thismod = ptr_cast<MPModVarData*>(member);
		//	addChild(new CNodeVarElem(name, thismod->pval, thismod->aptr));

			// 変数要素ではなく値をノードにする
			PVal* const pval = thismod->pval;
			pval->offset = thismod->aptr;

			auto const hvp = hpimod::getHvp(pval->flag);
			auto const fv = ptr_cast<FlexValue*>(hvp->GetPtr(pval));
			addChild(new CNodeModInst(fv));
			break;
		}
	//	case MPTYPE_STRING:
		case MPTYPE_LOCALSTRING:
		{
			addChild( new CNodeString(*ptr_cast<char const**>(member)) );
			break;
		}
		case MPTYPE_LABEL:
		{
			addChild( new CNodeLabel( *ptr_cast<label_t*>(member) ) );
			break;
		}
		case MPTYPE_DNUM:
		{
			addChild( new CNodeDouble( *ptr_cast<double*>(member) ) );
			break;
		}
		case MPTYPE_INUM:
		{
			addChild( new CNodeInt( *ptr_cast<int*>(member) ) );
			break;
		}

		case MPTYPE_STRUCTTAG: break;

		// 他 => 無視
		default:
			addChild(new CNodeUnknown(member, stprm->mptype));
			break;
	}
	return;
}

}
