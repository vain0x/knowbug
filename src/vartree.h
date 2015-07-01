
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

void init( HWND hTree );
void term();

void addNode( HWND hwndTree, HTREEITEM hParent, CVarTree const& tree );
void addNodeSysvar( HWND hwndTree, HTREEITEM hParent );
LRESULT   customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd );
vartype_t getVartype( HWND hwndTree, HTREEITEM hItem );
string    getItemVarText( HWND hwndTree, HTREEITEM hItem );

static inline bool isModuleNode( char const* name ) { return name[0] == '@' || name[0] == '+'; }
static inline bool isSysvarNode( char const* name ) { return name[0] == '~'; }
static inline bool isCallNode  ( char const* name ) { return name[0] == '\''; }
static inline bool isResultNode( char const* name ) { return name[0] == '"'; }
static inline bool isVarNode   ( char const* name ) { return !(isModuleNode(name) || isSysvarNode(name) || isCallNode(name) || isResultNode(name)); }

#ifdef with_WrapCall

void addNodeDynamic( HWND hwndTree, HTREEITEM hParent );
void CallTree_RemoveDependResult( HWND hwndTree, HTREEITEM hItem );

void AddCallNode ( HWND hwndTree, ModcmdCallInfo const& callinfo );
void RemoveCallNode( HWND hwndTree, ModcmdCallInfo const& callinfo );
ResultNodeData const* AddResultNode( HWND hwndTree, ModcmdCallInfo const& callinfo, void* ptr, vartype_t flag );
void RemoveResultNode( HWND hwndTree, HTREEITEM hResult );
void UpdateCallNode(HWND hwndTree);

}

#endif
