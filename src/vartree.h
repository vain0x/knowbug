
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
#endif
#include "VarTreeNodeData.h"

namespace VarTree {

void init();
void term();
void update();
void updateViewWindow();

void saveCurrentViewCaret(int vcaret);
int viewCaretFromNode(HTREEITEM hItem);

LRESULT customDraw(LPNMTVCUSTOMDRAW pnmcd);
auto getItemVarText(HTREEITEM hItem) -> shared_ptr<string const>;
auto tryGetNodeData(HTREEITEM hItem) -> shared_ptr<VTNodeData>;

void selectNode(VTNodeData const&);

} //namespace VarTree
