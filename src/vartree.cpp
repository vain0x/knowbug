
#include <Windows.h>
#include <CommCtrl.h>

#include "module/GuiUtility.h"

#include "main.h"
#include "DebugInfo.h"
#include "dialog.h"
#include "config_mng.h"

#include "vartree.h"
#include "StaticVarTree.h"
#include "CVarinfoText.h"
#include "CVardataString.h"

#include "WrapCall/WrapCall.h"
#include "WrapCall/ModcmdCallInfo.h"

#define hwndVarTree (Dialog::getVarTreeHandle())

namespace VarTree
{

static vartype_t getVartypeOfNode(HTREEITEM hItem);
static void AddNodeModule(HTREEITEM hParent, StaticVarTree const& tree);
static HTREEITEM AddNodeSystem(char const* name, SystemNodeId id);
static void AddNodeSysvar();

static HTREEITEM g_hNodeScript, g_hNodeLog;
HTREEITEM getScriptNodeHandle() { return g_hNodeScript; }
HTREEITEM getLogNodeHandle() { return g_hNodeLog; }

#ifdef with_WrapCall
using WrapCall::ModcmdCallInfo;

static HTREEITEM g_hNodeDynamic;
static void AddNodeDynamic();

// ツリービューに含まれる返値ノードのデータ
using resultDataPtr_t = shared_ptr<ResultNodeData const>;
static std::map<HTREEITEM, resultDataPtr_t> g_allResultData;
static HTREEITEM g_lastIndependedResultNode; // 非依存な返値ノード

struct UnreflectedDynamicNodeInfo {
	size_t countCallNodes; // 次の更新で追加すべき呼び出しノードの個数
	std::vector<resultDataPtr_t> resultNodes; // 次の更新で追加すべき返値ノード
	resultDataPtr_t resultNodeIndepended;     // 次の更新で追加すべき非依存な返値ノード
};
static UnreflectedDynamicNodeInfo g_unreflectedDynamicNodeInfo;

static void AddCallNodeImpl(ModcmdCallInfo::shared_ptr_type const& callinfo);
static void AddResultNodeImpl(resultDataPtr_t const& pResult);
static HTREEITEM FindDependedCallNode(resultDataPtr_t const& pResult);
static void RemoveDependingResultNodes(HTREEITEM hItem);
static void RemoveLastIndependedResultNode();
static auto FindLastIndependedResultData() -> shared_ptr<ResultNodeData const>;
#endif

//------------------------------------------------
// ツリービューのノードの LPARAM の型定義
//------------------------------------------------
bool VarNode::isTypeOf(char const* s)
{
	return !(ModuleNode::isTypeOf(s) || SystemNode::isTypeOf(s) || SysvarNode::isTypeOf(s)
#ifdef with_WrapCall
	|| InvokeNode::isTypeOf(s) || ResultNode::isTypeOf(s)
#endif //defined(with_WrapCall)
	);
}

namespace Detail
{
	template<> struct Verify < ModuleNode > { static bool apply(char const*, ModuleNode::lparam_t value) {
		return (value != nullptr);
	} };
	template<> struct Verify < SystemNode > { static bool apply(char const*, SystemNode::lparam_t value) {
		return true;
	} };
	template<> struct Verify < SysvarNode > { static bool apply(char const* s, SysvarNode::lparam_t value) {
		return (0 <= value && value < hpiutil::Sysvar::Count && hpiutil::Sysvar::trySeek(&s[1]) == value);
	} };
#ifdef with_WrapCall
	template<> struct Verify < InvokeNode > { static bool apply(char const*, InvokeNode::lparam_t value) {
		return (value >= 0);
	} };
	template<> struct Verify < ResultNode > { static bool apply(char const*, ResultNode::lparam_t value) {
		return true;
	} };
#endif //defined(with_WrapCall)
	template<> struct Verify < VarNode > { static bool apply(char const*, PVal* pval) {
		return (pval && hpiutil::staticVars().begin() <= pval && pval < hpiutil::staticVars().end());
	} };
}

template<typename Tag, typename lparam_t>
lparam_t TreeView_MyLParam(HWND hTree, HTREEITEM hItem, Tag*)
{
	auto&& v = (lparam_t)TreeView_GetItemLParam(hTree, hItem);
	DbgArea {
		string const&& s = TreeView_GetItemString(hTree, hItem);
		assert(Tag::isTypeOf(s.c_str()) && Detail::Verify<Tag>::apply(s.c_str(), v));
	}
	return v;
}

//------------------------------------------------
// 変数ツリーの初期化
//------------------------------------------------
void init()
{
	AddNodeModule(TVI_ROOT, StaticVarTree::Global::instance());
#ifdef with_WrapCall
	AddNodeDynamic();
#endif
	AddNodeSysvar();
	g_hNodeScript = AddNodeSystem("+script", SystemNodeId::Script);
	g_hNodeLog    = AddNodeSystem("+log", SystemNodeId::Log);
	AddNodeSystem("+general", SystemNodeId::General);

	//@, +dynamic は開いておく
#ifdef with_WrapCall
	TreeView_Expand(hwndVarTree, g_hNodeDynamic, TVE_EXPAND);
#endif
	HTREEITEM const hRoot = TreeView_GetRoot(hwndVarTree);
	assert(TreeView_GetItemString(hwndVarTree, hRoot) == StaticVarTree::Global::Name);
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

		// dynamic 関連のデータを削除する (必要なさそう)
		if ( usesResultNodes() ) {
			g_unreflectedDynamicNodeInfo = UnreflectedDynamicNodeInfo {};
		}
	}
#endif
}

//------------------------------------------------
// ツリービューに要素を挿入する
//------------------------------------------------
template<typename Tag>
static HTREEITEM TreeView_MyInsertItem(HTREEITEM hParent, char const* name, bool sorts, typename Tag::lparam_t lp, Tag* = nullptr)
{
	TVINSERTSTRUCT tvis {};
	tvis.hParent = hParent;
	tvis.hInsertAfter = (sorts ? TVI_SORT : TVI_LAST);
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM;
	tvis.item.pszText = const_cast<char*>(name);
	tvis.item.lParam = (LPARAM)(lp);
	return TreeView_InsertItem(hwndVarTree, &tvis);
}

//------------------------------------------------
// 変数ツリーにノードを追加する
//------------------------------------------------
void AddNodeModule(HTREEITEM hParent, StaticVarTree const& tree)
{
	auto const hElem = TreeView_MyInsertItem<ModuleNode>(hParent, tree.getName().c_str(), true, &tree);
	tree.foreach(
		[&](StaticVarTree const& module) {
			AddNodeModule(hElem, module);
		},
		[&](string const& varname) {
			PVal* const pval = hpiutil::seekSttVar(varname.c_str());
			assert(!!pval);
			TreeView_MyInsertItem<VarNode>(hElem, varname.c_str(), true, pval);
		}
	);
}

//------------------------------------------------
// 変数ツリーにシステムノードを追加する
//------------------------------------------------
HTREEITEM AddNodeSystem(char const* name, SystemNodeId id) {
	assert(SystemNode::isTypeOf(name));
	return TreeView_MyInsertItem<SystemNode>(TVI_ROOT, name, false, id);
}

//------------------------------------------------
// 変数ツリーにシステム変数ノードを追加する
//------------------------------------------------
void AddNodeSysvar()
{
	using namespace hpiutil;

	HTREEITEM const hNodeSysvar = AddNodeSystem("+sysvar", SystemNodeId::Sysvar);

	// システム変数のリストを追加する
	for ( int i = 0; i < Sysvar::Count; ++ i ) {
		string const name = strf( "~%s", Sysvar::List[i].name );
		TreeView_MyInsertItem<SysvarNode>(hNodeSysvar, name.c_str(), false, static_cast<Sysvar::Id>(i));
	}
}

#ifdef with_WrapCall
//------------------------------------------------
// 変数ツリーに動的変数ノードを追加する
//------------------------------------------------
void AddNodeDynamic()
{
	g_hNodeDynamic = AddNodeSystem("+dynamic", SystemNodeId::Dynamic);
}
#endif

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

#ifdef with_WrapCall
	if ( InvokeNode::isTypeOf(name) ) {
		auto const idx = TreeView_MyLParam<InvokeNode>(hwndVarTree, hItem);
		if ( auto const pCallInfo = WrapCall::tryGetCallInfoAt(idx) ) {
			auto const key = (pCallInfo->stdat->index == STRUCTDAT_INDEX_FUNC)
				? "__sttm__"
				: "__func__";
			auto const&& iter = g_config->clrTextExtra.find(key);
			if ( iter != g_config->clrTextExtra.end() ) {
				return cont(iter->second);
			}
		}
	} else
#endif //defined(with_WrapCall)
	{
		vartype_t const vtype = getVartypeOfNode(hItem);
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
// 変数ツリーの要素の型を取得する
//------------------------------------------------
vartype_t getVartypeOfNode( HTREEITEM hItem )
{
	string const&& name = TreeView_GetItemString(hwndVarTree, hItem);

	if ( VarNode::isTypeOf(name.c_str()) ) {
		PVal* const pval = TreeView_MyLParam<VarNode>(hwndVarTree, hItem);
		return pval->flag;

	} else if ( SysvarNode::isTypeOf(name.c_str()) ) {
		auto const id = TreeView_MyLParam<SysvarNode>(hwndVarTree, hItem);
		return hpiutil::Sysvar::List[id].type;

#ifdef with_WrapCall
	} else if ( ResultNode::isTypeOf(name.c_str()) ) {
		auto const&& iter = g_allResultData.find(hItem);
		if ( iter != g_allResultData.end() ) {
			return iter->second->vtype;
		}
#endif //defined(with_WrapCall)
	}
	return HSPVAR_FLAG_NONE;
}

//------------------------------------------------
// 変数情報のテキストを取得する
//------------------------------------------------
std::shared_ptr<string const> getItemVarText( HTREEITEM hItem )
{
	string const&& itemText = TreeView_GetItemString( hwndVarTree, hItem );
	char const* const name = itemText.c_str();

	CVarinfoText varinf;

	if ( ModuleNode::isTypeOf(name) ) {
		auto const pTree = TreeView_MyLParam<ModuleNode>(hwndVarTree, hItem);
		varinf.addModuleOverview(name, *pTree);

	} else if ( SystemNode::isTypeOf(name) ) {
		switch ( TreeView_MyLParam<SystemNode>(hwndVarTree, hItem) ) {
#ifdef with_WrapCall
			case SystemNodeId::Dynamic:
				varinf.addCallsOverview(FindLastIndependedResultData().get());
				break;
#endif
			case SystemNodeId::Sysvar:
				varinf.addSysvarsOverview();
				break;
			case SystemNodeId::Log:
				return shared_ptr_from_rawptr(&Dialog::LogBox::get());

			case SystemNodeId::Script:
				if ( auto const p = Dialog::tryGetCurrentScript() ) {
					return shared_ptr_from_rawptr(p);
				} else {
					return std::make_shared<string>(g_dbginfo->getCurInfString());
				}
			case SystemNodeId::General:
				varinf.addGeneralOverview();
				break;
			default: assert_sentinel;
		}
	} else {
		if ( SysvarNode::isTypeOf(name) ) {
			auto const id = static_cast<hpiutil::Sysvar::Id>(TreeView_GetItemLParam(hwndVarTree, hItem));
			varinf.addSysvar(id);

#ifdef with_WrapCall
		} else if ( InvokeNode::isTypeOf(name) ) {
			auto const idx = TreeView_MyLParam<InvokeNode>( hwndVarTree, hItem );
			if ( auto const pCallInfo = WrapCall::tryGetCallInfoAt(idx) ) {
				varinf.addCall(*pCallInfo);
			}

		} else if ( usesResultNodes() && ResultNode::isTypeOf(name) ) {
			auto const&& iter = g_allResultData.find(hItem);
			auto const pResult = (iter != g_allResultData.end() ? iter->second : nullptr);
			varinf.addResult(*pResult);
#endif
		} else {
			assert(VarNode::isTypeOf(name));
			PVal* const pval = TreeView_MyLParam<VarNode>(hwndVarTree, hItem);
			varinf.addVar( pval, name );
		}
	}
	return std::make_shared<string>(varinf.getStringMove());
}

#ifdef with_WrapCall
//------------------------------------------------
// 呼び出しノードを追加
//------------------------------------------------
void AddCallNode(ModcmdCallInfo::shared_ptr_type const& callinfo)
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
	char name[128] = "'";
	strcpy_s(&name[1], sizeof(name) - 1, hpiutil::STRUCTDAT_name(callinfo->stdat));
	HTREEITEM const hChild = TreeView_MyInsertItem<InvokeNode>(g_hNodeDynamic, name, false, callinfo->idx);

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
		assert( InvokeNode::isTypeOf(TreeView_GetItemString(hwndVarTree, hLast).c_str()) );

		TreeView_EscapeFocus(hwndVarTree, hLast);
		RemoveDependingResultNodes(hLast);
		TreeView_DeleteItem(hwndVarTree, hLast);
	}
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
	char name[128] = "\"";
	strcpy_s( &name[1], sizeof(name) - 1, hpiutil::STRUCTDAT_name(pResult->callinfo->stdat) );
	HTREEITEM const hChild = TreeView_MyInsertItem<ResultNode>(hParent, name, false, nullptr);

	// 第一ノードなら自動的に開く
	if ( TreeView_GetChild( hwndVarTree, hParent ) == hChild ) {
		TreeView_Expand( hwndVarTree, hParent, TVE_EXPAND );
	}

	// 返値ノードデータを保存しておく
	g_allResultData.emplace(hChild, pResult);

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
			if ( !InvokeNode::isTypeOf(TreeView_GetItemString(hwndVarTree, hItem).c_str()) ) continue;
			int const idx = TreeView_MyLParam<InvokeNode>(hwndVarTree, hItem);
			if ( WrapCall::tryGetCallInfoAt(idx) == pResult->pCallInfoDepended ) break;
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

	// 関連していた返値ノードデータを破棄
	{
		size_t const cnt = g_allResultData.erase(hResult);
		assert(cnt == 1);
	}

	TreeView_DeleteItem(hwndVarTree, hResult);
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

	DbgArea {
		string const nodeName = TreeView_GetItemString(hwndVarTree, hItem);
		assert(InvokeNode::isTypeOf(nodeName.c_str()) || ResultNode::isTypeOf(nodeName.c_str()));
	};

	for ( HTREEITEM hChild = TreeView_GetChild(hwndVarTree, hItem)
		; hChild != nullptr
		;
	) {
		HTREEITEM const hNext = TreeView_GetNextSibling(hwndVarTree, hChild);
		string const nodeName = TreeView_GetItemString(hwndVarTree, hChild);
		if ( ResultNode::isTypeOf(nodeName.c_str()) ) {
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
auto FindLastIndependedResultData() -> shared_ptr<ResultNodeData const>
{
	if ( !g_lastIndependedResultNode ) return nullptr;
	auto const&& iter = g_allResultData.find(g_lastIndependedResultNode);
	return (iter != g_allResultData.end())
		? iter->second
		: nullptr;
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
