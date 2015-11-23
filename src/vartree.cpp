
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"

#include "vartree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#define hwndVarTree (Dialog::getVarTreeHandle())

namespace VarTree
{

static HTREEITEM g_hNodeScript, g_hNodeLog;
HTREEITEM getScriptNodeHandle() { return g_hNodeScript; }
HTREEITEM getLogNodeHandle() { return g_hNodeLog; }

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;

static HTREEITEM g_hNodeDynamic;

using resultDataPtr_t = shared_ptr<ResultNodeData>;
static HTREEITEM g_lastIndependedResultNode; // 非依存な返値ノード

static void AddResultNode(ModcmdCallInfo::shared_ptr_type const& callinfo, resultDataPtr_t const& pResult);

#endif

static auto TreeView_MyInsertItem
	( HTREEITEM hParent, char const* name, bool sorts
	, shared_ptr<VTNodeData> node) -> HTREEITEM;
static void TreeView_MyDeleteItem(HTREEITEM hItem);

static auto makeNodeName(VTNodeData const& node) -> string;

// TvRepr
class TvRepr
{
public:
	TvRepr()
	{
		observer_ = std::make_shared<TvObserver>(*this);
		VTNodeData::registerObserver(observer_);
	}
	~TvRepr()
	{
		VTNodeData::unregisterObserver(observer_);
	}

	auto itemFromNode(VTNodeData const* p) -> HTREEITEM
	{
		auto&& iter = itemFromNode_.find(p);
		return (iter != itemFromNode_.end()) ? iter->second : nullptr;
	}

private:
	struct TvObserver;
	friend struct TvObserver;

	struct TvObserver
		: VTNodeData::Observer
	{
		TvRepr& self;

	public:
		TvObserver(TvRepr& self)
			: self(self)
		{
			self.itemFromNode_[&VTNodeRoot::instance()] = TVI_ROOT;
		}

		void onInit(VTNodeData& node) override
		{
			if ( !node.parent() ) return; // VTNodeRoot

			auto&& hParent = self.itemFromNode(node.parent().get());
			auto&& hItem = TreeView_MyInsertItem
				( hParent
				, makeNodeName(node).c_str()
				, false
				, node.shared_from_this());

			assert(self.itemFromNode_[&node] == nullptr);
			self.itemFromNode_[&node] = hItem;

			// TODO: @, +dynamic, 呼び出しノードは自動的に開く
		}
		void onTerm(VTNodeData& node) override
		{
			if ( auto&& hItem = self.itemFromNode(&node) ) {
				self.itemFromNode_[&node] = nullptr;
				TreeView_MyDeleteItem(hItem);
			}
		}
	};

private:
	std::map<VTNodeData const*, HTREEITEM> itemFromNode_;
	shared_ptr<TvObserver> observer_;
};

static std::unique_ptr<TvRepr> g_tv;

//------------------------------------------------
// 変数ツリーの初期化
//------------------------------------------------
void init()
{
	g_tv.reset(new TvRepr());

	VTNodeRoot::make_shared()->updateDeep();

#ifdef with_WrapCall
	g_hNodeDynamic = g_tv->itemFromNode(VTNodeDynamic::make_shared().get());
#endif
	g_hNodeScript  = g_tv->itemFromNode(VTNodeScript::make_shared().get());
	g_hNodeLog     = g_tv->itemFromNode(VTNodeLog::make_shared().get());
	
	//@, +dynamic は開いておく
#ifdef with_WrapCall
	TreeView_Expand(hwndVarTree, g_hNodeDynamic, TVE_EXPAND);
#endif
	HTREEITEM const hRoot = TreeView_GetRoot(hwndVarTree);
	assert(TreeView_GetItemString(hwndVarTree, hRoot) == VTNodeModule::Global::Name);
	TreeView_Expand(hwndVarTree, hRoot, TVE_EXPAND);

	TreeView_EnsureVisible(hwndVarTree, hRoot); //トップまでスクロール
	TreeView_SelectItem(hwndVarTree, hRoot);
}

//------------------------------------------------
// 変数ツリー終了時
//------------------------------------------------
void term()
{
	g_tv.reset();
}

void update()
{
#ifdef with_WrapCall
	VTNodeDynamic::make_shared()->updateDeep();
#endif
}

//------------------------------------------------
// ツリービューに要素を挿入する
//------------------------------------------------
static HTREEITEM TreeView_MyInsertItem
	( HTREEITEM hParent
	, char const* name
	, bool sorts
	, shared_ptr<VTNodeData> node)
{
	TVINSERTSTRUCT tvis {};
	tvis.hParent = hParent;
	tvis.hInsertAfter = (sorts ? TVI_SORT : TVI_LAST);
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.lParam  = (LPARAM)node.get();
	tvis.item.pszText = const_cast<char*>(name);

	return TreeView_InsertItem(hwndVarTree, &tvis);
}

auto tryGetNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>
{
	auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
	assert(lp);
	try {
		return lp->shared_from_this();
	} catch ( std::bad_weak_ptr const& ) {
		return nullptr;
	}
}

static void TreeView_MyDeleteItem(HTREEITEM hItem)
{
	TreeView_EscapeFocus(hwndVarTree, hItem);
	TreeView_DeleteItem(hwndVarTree, hItem);
}

// ノードにつけるべき名前
auto makeNodeName(VTNodeData const& node) -> string
{
	struct matcher : VTNodeData::Visitor
	{
		string result;
		string apply(VTNodeData const& node)
		{
			result = node.name(); // default
			node.acceptVisitor(*this);
			return std::move(result);
		}

		void fInvoke(VTNodeInvoke const& node) override { result = "\'" + node.name(); }
		void fResult(VTNodeResult const& node) override { result = "\"" + node.name(); }
	};

	return matcher {}.apply(node);
}

//------------------------------------------------
// ノードに対応する文字色
//
// Return true iff text color is modified.
//------------------------------------------------
static bool customizeTextColorIfAble(HTREEITEM hItem, LPNMTVCUSTOMDRAW pnmcd)
{
	// 選択状態なら色分けしない
	if ( TreeView_GetItemState(hwndVarTree, hItem, 0) & TVIS_SELECTED ) {
		return false;
	}

	string const sItem = TreeView_GetItemString(hwndVarTree, hItem);
	char const* const name = sItem.c_str();

	auto const cont = [&pnmcd](COLORREF cref) {
		pnmcd->clrText = cref;
		return true;
	};

	auto const node = tryGetNodeData(hItem);
	if ( !node ) return false;

#ifdef with_WrapCall
	if ( auto const nodeInvoke = std::dynamic_pointer_cast<VTNodeInvoke const>(node) ) {
			auto const key = (nodeInvoke->callinfo().stdat->index == STRUCTDAT_INDEX_FUNC)
				? "__sttm__"
				: "__func__";
			auto const&& iter = g_config->clrTextExtra.find(key);
			if ( iter != g_config->clrTextExtra.end() ) {
				return cont(iter->second);
			}
	} else
#endif //defined(with_WrapCall)
	{
		vartype_t const vtype = node->vartype();
		if ( 0 < vtype && vtype < HSPVAR_FLAG_USERDEF ) {
			return cont(g_config->clrText[vtype]);

		} else if ( vtype >= HSPVAR_FLAG_USERDEF ) {
			auto const&& iter = g_config->clrTextExtra.find(hpiutil::varproc(vtype)->vartype_name);
			if ( iter != g_config->clrTextExtra.end() ) {
				return cont(iter->second);
			}
		}
	}
	return false;
}

//------------------------------------------------
// 変数ツリーの NM_CUSTOMDRAW を処理する
//------------------------------------------------
LRESULT customDraw( LPNMTVCUSTOMDRAW pnmcd )
{
	if ( pnmcd->nmcd.dwDrawStage == CDDS_PREPAINT ) {
		return CDRF_NOTIFYITEMDRAW;

	} else if ( pnmcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT ) {
		auto const hItem = reinterpret_cast<HTREEITEM>(pnmcd->nmcd.dwItemSpec);
		bool const modified = customizeTextColorIfAble(hItem, pnmcd);
		if ( modified ) {
			return CDRF_NEWFONT;
		}
	}
	return 0;
}

//------------------------------------------------
// 変数情報のテキストを取得する
//------------------------------------------------
std::shared_ptr<string const> getItemVarText( HTREEITEM hItem )
{
	struct GetText
		: public VTNodeData::Visitor
	{
		CVarinfoText varinf;
		shared_ptr<string const> result;

		void fModule(VTNodeModule const& node) override
		{
			varinf.addModuleOverview(node.name().c_str(), node);
		}
		void fVar(VTNodeVar const& node) override
		{
			varinf.addVar(node.pval(), node.name().c_str());
		}
		void fSysvarList(VTNodeSysvarList const&) override
		{
			varinf.addSysvarsOverview();
		}
		void fLog(VTNodeLog const&) override
		{
			result = shared_ptr_from_rawptr(&Dialog::LogBox::get());
		}
		void fScript(VTNodeScript const&) override
		{
			if ( auto const p = Dialog::tryGetCurrentScript() ) {
				result = shared_ptr_from_rawptr(p);
			} else {
				result = std::make_shared<string>(g_dbginfo->getCurInfString());
			}
		}
		void fGeneral(VTNodeGeneral const&) override
		{
			varinf.addGeneralOverview();
		}
		void fSysvar(VTNodeSysvar const& node) override
		{
			varinf.addSysvar(node.id());
		}
#ifdef with_WrapCall
		void fDynamic(VTNodeDynamic const&) override
		{
			varinf.addCallsOverview(VTNodeDynamic::make_shared()->lastIndependedResult().get());
		}
		void fInvoke(VTNodeInvoke const& node) override
		{
			varinf.addCall(node.callinfo());
		}
		void fResult(VTNodeResult const& node) override
		{
			assert(usesResultNodes());
			varinf.addResult(node);
		}
#endif
		auto apply(VTNodeData const& node) -> shared_ptr<string const>
		{
			node.acceptVisitor(*this);
			return (result)
				? result
				: std::make_shared<string>(varinf.getStringMove());
		}
	};

	if ( auto&& node = tryGetNodeData(hItem) ) {
		return GetText {}.apply(*node);
	} else {
		return std::make_shared<string>("(not_available)");
	}
}

#ifdef with_WrapCall

void OnBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	auto&& node = std::make_shared<VTNodeInvoke>(callinfo);
	VTNodeDynamic::make_shared()->addInvokeNode(std::move(node));
}

auto OnEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vtype)
	-> shared_ptr<ResultNodeData const>
{
	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、他のことをする前に、文字列化などの処理を済ませておく必要がある。
	auto&& pResult =
		(usesResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
		? std::make_shared<ResultNodeData>(callinfo, ptr, vtype)
		: nullptr;

	VTNodeDynamic::make_shared()->eraseLastInvokeNode();
	
	if ( pResult ) {
		AddResultNode(callinfo, pResult);
	}
	return pResult;
}

/**
返値ノードを追加

「A( B() )」のように、ユーザ定義コマンドの引数式の中でユーザ定義関数が呼ばれている状態を、
「A は B に依存する」と表現することにする。A もユーザ定義関数である場合のみ考える。
このとき B の実行が終了してから A の実行が始まる。
B の返値ノードは、A の呼び出しノードの子ノードとして追加される。

表示がごちゃごちゃしないように、返値ノードは近いうちに削除される。
具体的には、以下の通り：
1. 非依存な返値ノードは、次に呼び出しノードか非依存な返値ノードが追加される直前、
	または呼び出しノードが削除される直前に取り除かれる。
2. 依存する返値ノードは、その依存先の呼び出しノードが削除されるときに取り除かれる。
3. 実行が終了したとき、すべての返値ノードが取り除かれる。
*/

void AddResultNode(ModcmdCallInfo::shared_ptr_type const& callinfo, resultDataPtr_t const& pResult)
{
	assert(!!pResult);

	if ( auto&& node = pResult->dependedNode() ) {
		node->addResultDepended(pResult);
	} else {
		VTNodeDynamic::make_shared()->addResultNodeIndepended(pResult);
	}
}

#endif //defined(with_WrapCall)

} // namespace VarTree
