
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#include "module/strf.h"
#include "CVarTree.h"

#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
# include "WrapCall/ResultNodeData.h"
using WrapCall::ModcmdCallInfo;
using WrapCall::ResultNodeData;
#endif

namespace VarTree
{

void init();
void term();

LRESULT   customDraw(LPNMTVCUSTOMDRAW pnmcd);
vartype_t getVartype(HTREEITEM hItem );
string    getItemVarText(HTREEITEM hItem);

static inline bool isModuleNode(char const* name) { return name[0] == '@'; }
static inline bool isSystemNode(char const* name) { return name[0] == '+'; }
static inline bool isSysvarNode(char const* name) { return name[0] == '~'; }
static inline bool isCallNode  (char const* name) { return name[0] == '\''; }
static inline bool isResultNode(char const* name) { return name[0] == '"'; }
static inline bool isVarNode   (char const* name) {
	return !(isModuleNode(name) || isSystemNode(name) || isSysvarNode(name) || isCallNode(name) || isResultNode(name));
}

#ifdef with_WrapCall

void AddCallNode(ModcmdCallInfo const& callinfo);
void RemoveLastCallNode();
void AddResultNode(ModcmdCallInfo const& callinfo, std::shared_ptr<ResultNodeData> pResult);
void RemoveResultNode( HTREEITEM hResult );
void UpdateCallNode();

}

#endif
