
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

// ツリービューに含まれる返値ノードのデータ
using resultDataPtr_t = shared_ptr<ResultNodeData>;
static HTREEITEM g_lastIndependedResultNode; // 非依存な返値ノード

struct UnreflectedDynamicNodeInfo {
	size_t countCallNodes; // 次の更新で追加すべき呼び出しノードの個数
	std::vector<resultDataPtr_t> resultNodes; // 次の更新で追加すべき返値ノード
	resultDataPtr_t resultNodeIndepended;     // 次の更新で追加すべき非依存な返値ノード
};
static UnreflectedDynamicNodeInfo g_unreflectedDynamicNodeInfo;

static void UpdateCallNode();
static void AddCallNodeImpl(ModcmdCallInfo::shared_ptr_type const& callinfo);
static void AddResultNode(ModcmdCallInfo::shared_ptr_type const& callinfo, resultDataPtr_t const& pResult);
static void AddResultNodeImpl(resultDataPtr_t const& pResult);
static HTREEITEM FindDependedCallNode(resultDataPtr_t const& pResult);
static void RemoveDependingResultNodes(HTREEITEM hItem);
static void RemoveLastIndependedResultNode();
static auto FindLastIndependedResultData() -> resultDataPtr_t;
#endif

static auto TreeView_MyInsertItem
	( HTREEITEM hParent, char const* name, bool sorts
	, shared_ptr<VTNodeData> node) -> HTREEITEM;
static void TreeView_MyDeleteItem(HTREEITEM hItem);

// TvRepr
class TvRepr
{
public:
	TvRepr()
	{
		VTNodeData::registerObserver(std::make_shared<TvObserver>(*this));
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
		TvObserver(TvRepr& self) : self(self) {}

		void onInit(VTNodeData& node) override
		{
			auto&& hParent = self.itemFromNode(node.parent().get());
			auto&& hItem = TreeView_MyInsertItem
				( (hParent ? hParent : TVI_ROOT)
				, node.name().c_str()
				, false
				, node.shared_from_this());
			self.itemFromNode_.emplace(&node, hItem);
		}
		void onTerm(VTNodeData& node) override
		{
			if ( auto&& hItem = self.itemFromNode(&node) ) {
				TreeView_MyDeleteItem(hItem);
			}
		}
	};

private:
	std::map<VTNodeData const*, HTREEITEM> itemFromNode_;
};

static std::unique_ptr<TvRepr> g_tv;

//------------------------------------------------
// 変数ツリーの初期化
//------------------------------------------------
void init()
{
	g_tv.reset(new TvRepr());

	static auto roots = vector<shared_ptr<VTNodeData>>
		{ VTNodeModule::Global::make_shared()
#ifdef with_WrapCall
		, VTNodeDynamic::make_shared()
#endif
		, VTNodeSysvarList::make_shared()
		, VTNodeScript::make_shared()
		, VTNodeLog::make_shared()
		, VTNodeGeneral::make_shared()
		};
	for ( auto&& e : roots ) {
		e->updateDeep();
	}

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
#ifdef with_WrapCall
	if ( auto const hVarTree = Dialog::getVarTreeHandle() ) {
		RemoveDependingResultNodes(g_hNodeDynamic);

		if ( usesResultNodes() ) {
			g_unreflectedDynamicNodeInfo = UnreflectedDynamicNodeInfo {};
		}
	}
#endif

	g_tv.reset();
}

void update()
{
	UpdateCallNode();
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

auto getNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>
{
	auto const lp = reinterpret_cast<VTNodeData*>(TreeView_GetItemLParam(hwndVarTree, hItem));
	assert(lp);
	auto&& p = lp->shared_from_this();
	assert(p);
	return p;
}

static void TreeView_MyDeleteItem(HTREEITEM hItem)
{
	TreeView_DeleteItem(hwndVarTree, hItem);
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

	auto const node = getNodeData(hItem);
	if ( !node ) return false;

#ifdef with_WrapCall
	if ( auto const pCallInfo = std::dynamic_pointer_cast<VTNodeInvoke const>(node) ) {
			auto const key = (pCallInfo->stdat->index == STRUCTDAT_INDEX_FUNC)
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
			varinf.addCallsOverview(FindLastIndependedResultData().get());
		}
		void fInvoke(VTNodeInvoke const& node) override
		{
			varinf.addCall(node);
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

	return GetText {}.apply(*getNodeData(hItem));
}

#ifdef with_WrapCall
//------------------------------------------------
// 呼び出しノードを追加
//------------------------------------------------
void OnBgnCalling(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	if ( usesResultNodes() ) {
		RemoveLastIndependedResultNode();
	}

	if ( Knowbug::isStepRunning() ) {
		AddCallNodeImpl(callinfo);
	} else {
		// 次に停止したときにまとめて追加する
		++g_unreflectedDynamicNodeInfo.countCallNodes;
	}
}

void AddCallNodeImpl(ModcmdCallInfo::shared_ptr_type const& callinfo)
{
	string name = "'" + callinfo->name();
	HTREEITEM const hChild =
		TreeView_MyInsertItem(g_hNodeDynamic, name.c_str(), false, callinfo);

	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndVarTree, g_hNodeDynamic ) == hChild ) {
		TreeView_Expand( hwndVarTree, g_hNodeDynamic, TVE_EXPAND );
	}
}

//------------------------------------------------
// 最後の呼び出しノードを削除
//------------------------------------------------
void RemoveLastCallNode()
{
	if ( g_unreflectedDynamicNodeInfo.countCallNodes > 0 ) {
		--g_unreflectedDynamicNodeInfo.countCallNodes;

	} else {
		// 末子に返値ノードがあれば削除する
		if ( usesResultNodes() ) {
			RemoveLastIndependedResultNode();
		}

		HTREEITEM const hLast = TreeView_GetChildLast(hwndVarTree, g_hNodeDynamic);
		if ( !hLast ) return;
		assert(std::dynamic_pointer_cast<VTNodeInvoke const>(getNodeData(hLast)));

		TreeView_EscapeFocus(hwndVarTree, hLast);
		RemoveDependingResultNodes(hLast);
		TreeView_MyDeleteItem(hLast);
	}
}

auto OnEndCalling(ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vtype)
	-> shared_ptr<ResultNodeData const>
{
	// 返値ノードデータの生成
	// ptr の生存期限が今だけなので、今作るしかない
	auto&& pResult =
		(usesResultNodes() && ptr != nullptr && vtype != HSPVAR_FLAG_NONE)
		? std::make_shared<ResultNodeData>(callinfo, ptr, vtype)
		: nullptr;
	
	RemoveLastCallNode();

	if ( pResult ) {
		AddResultNode(callinfo, pResult);
	}
	return pResult;
}

//------------------------------------------------
// 返値ノードを追加
/*
返値データ ptr の生存期間は今だけなので、今のうちに文字列化しなければいけない。
返値ノードも、呼び出しノードと同様に、次に実行が停止したときにまとめて追加する。

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
//------------------------------------------------
void AddResultNode(ModcmdCallInfo::shared_ptr_type const& callinfo, resultDataPtr_t const& pResult)
{
	assert(!!pResult);

	if ( Knowbug::isStepRunning() ) {
		AddResultNodeImpl(pResult);

	} else {
		if ( pResult->pCallInfoDepended ) {
			g_unreflectedDynamicNodeInfo.resultNodes.emplace_back(std::move(pResult));
		} else {
			g_unreflectedDynamicNodeInfo.resultNodeIndepended = std::move(pResult);
		}
	}
}

void AddResultNodeImpl(resultDataPtr_t const& pResult)
{
	HTREEITEM const hParent = FindDependedCallNode(pResult);
	if ( !hParent ) return;

	// 非依存な返値ノードは高々1個に限られる
	if ( hParent == g_hNodeDynamic ) {
		RemoveLastIndependedResultNode();
	}

	// 挿入
	string name = "\"" + pResult->name();
	HTREEITEM const hChild = TreeView_MyInsertItem(hParent, name.c_str(), false, pResult);

	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndVarTree, hParent ) == hChild ) {
		TreeView_Expand( hwndVarTree, hParent, TVE_EXPAND );
	}

	if ( hParent == g_hNodeDynamic ) {
		g_lastIndependedResultNode = hChild;
	}
}

//------------------------------------------------
// 依存元の呼び出しノードを探す (failure: nullptr)
//
// @ 依存元がツリービューになければ失敗とする。
//------------------------------------------------
HTREEITEM FindDependedCallNode(resultDataPtr_t const& pResult)
{
	// 依存されているなら、その呼び出しノードを検索する
	if ( pResult->pCallInfoDepended ) {
		HTREEITEM hItem = nullptr;
		for ( hItem = TreeView_GetChild(hwndVarTree, g_hNodeDynamic)
			; hItem != nullptr
			; hItem = TreeView_GetNextSibling(hwndVarTree, hItem)
		) {
			auto&& node = std::dynamic_pointer_cast<VTNodeInvoke const>(getNodeData(hItem));
			if ( !node ) continue;
			if ( WrapCall::tryGetCallInfoAt(node->idx) == pResult->pCallInfoDepended ) break;
		}
		return hItem;

	// 非依存なら、+dynamic 直下に追加する
	} else {
		return g_hNodeDynamic;
	}
}

//------------------------------------------------
// 返値ノードを削除
//------------------------------------------------
void RemoveResultNode(HTREEITEM hResult)
{
	// 現状返値ノードに依存する返値ノードは挿入されない
	//RemoveDependingResultNodes(hResult);

	TreeView_EscapeFocus(hwndVarTree, hResult);
	TreeView_MyDeleteItem(hResult);
}

//------------------------------------------------
// 依存している返値ノードをすべて削除する
//------------------------------------------------
static void RemoveDependingResultNodes(HTREEITEM hItem)
{
	if ( !usesResultNodes() ) return;

	// +dynamic 直下の返値ノードは非依存なものであり、それは末子の高々1つに限られる
	if ( hItem == g_hNodeDynamic ) {
		RemoveLastIndependedResultNode();
		return;
	}

	for ( HTREEITEM hChild = TreeView_GetChild(hwndVarTree, hItem)
		; hChild != nullptr
		;
	) {
		HTREEITEM const hNext = TreeView_GetNextSibling(hwndVarTree, hChild);
		auto&& lp = getNodeData(hChild);
		if ( auto const node = std::dynamic_pointer_cast<VTNodeResult const>(lp) ) {
			RemoveResultNode(hChild);
		}
		hChild = hNext;
	}
}

//------------------------------------------------
// (最後の)非依存な返値ノードを削除する
//------------------------------------------------
void RemoveLastIndependedResultNode()
{
	if ( g_lastIndependedResultNode ) {
		RemoveResultNode(g_lastIndependedResultNode);
		g_lastIndependedResultNode = nullptr;
	}
	g_unreflectedDynamicNodeInfo.resultNodeIndepended = nullptr;
}

//------------------------------------------------
// (最後の)非依存な返値ノードデータを探す
//------------------------------------------------
auto FindLastIndependedResultData() -> resultDataPtr_t
{
	if ( !g_lastIndependedResultNode ) return nullptr;
	return std::dynamic_pointer_cast<ResultNodeData>(
		getNodeData(g_lastIndependedResultNode));
}

//------------------------------------------------
// 呼び出しノード更新
//------------------------------------------------
void UpdateCallNode()
{
	// 追加予定の呼び出しノードを実際に追加する
	if ( g_unreflectedDynamicNodeInfo.countCallNodes > 0 ) {
		auto const&& range = WrapCall::getCallInfoRange();
		size_t const lenStk = std::distance(range.first, range.second);
		for ( size_t i = lenStk - g_unreflectedDynamicNodeInfo.countCallNodes; i < lenStk; ++i ) {
			AddCallNodeImpl(range.first[i]);
		}
		g_unreflectedDynamicNodeInfo.countCallNodes = 0;
	}

	// 追加予定の返値ノードを実際に追加する
	if ( usesResultNodes() ) {
		// 非依存なもの
		if ( g_unreflectedDynamicNodeInfo.resultNodeIndepended ) {
			resultDataPtr_t resultData = nullptr;
			swap(resultData, g_unreflectedDynamicNodeInfo.resultNodeIndepended);
			AddResultNodeImpl(resultData);
		}

		// 依存されているもの
		if ( !g_unreflectedDynamicNodeInfo.resultNodes.empty() ) {
			for ( auto const& pResult : g_unreflectedDynamicNodeInfo.resultNodes ) {
				AddResultNodeImpl(pResult);
			}
			g_unreflectedDynamicNodeInfo.resultNodes.clear();
		}
	}
}
#endif //defined(with_WrapCall)

} // namespace VarTree
