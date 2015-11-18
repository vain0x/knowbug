
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
#endif
#include "VarTreeNodeData.h"

class StaticVarTree;

namespace VarTree {

void init();
void term();

LRESULT customDraw(LPNMTVCUSTOMDRAW pnmcd);
std::shared_ptr<string const> getItemVarText(HTREEITEM hItem);

HTREEITEM getScriptNodeHandle();
HTREEITEM getLogNodeHandle();

auto TreeView_MyLParam(HWND hTree, HTREEITEM hItem) -> VTNodeData*;

#ifdef with_WrapCall

void AddCallNode(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
void RemoveLastCallNode();
void AddResultNode(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, std::shared_ptr<ResultNodeData> pResult);
void RemoveResultNode( HTREEITEM hResult );
void UpdateCallNode();

#endif

} //namespace VarTree
