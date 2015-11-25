
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
auto tryGetNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>;

HTREEITEM getScriptNodeHandle();
HTREEITEM getLogNodeHandle();

} //namespace VarTree
