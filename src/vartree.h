
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
void update();

LRESULT customDraw(LPNMTVCUSTOMDRAW pnmcd);
auto getItemVarText(HTREEITEM hItem) -> shared_ptr<string const>;
auto getNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>;

HTREEITEM getScriptNodeHandle();
HTREEITEM getLogNodeHandle();

#ifdef with_WrapCall

void OnBgnCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
auto OnEndCalling(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, PDAT const* ptr, vartype_t vtype) -> shared_ptr<ResultNodeData const>;

#endif

} //namespace VarTree
