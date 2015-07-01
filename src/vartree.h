
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#include "module/mod_cstring.h"
#include "CVarTree.h"

#include "WrapCall.h"

namespace VarTree
{

void init( HWND hTree );
void term();

void addNode( HWND hwndTree, HTREEITEM hParent, CVarTree& tree );
void addNodeSysvar( HWND hwndTree, HTREEITEM hParent );
LRESULT   customDraw( HWND hwndTree, LPNMTVCUSTOMDRAW pnmcd );
vartype_t getVartype( HWND hwndTree, HTREEITEM hItem );
string    getItemVarText( HWND hwndTree, HTREEITEM hItem );

static inline bool isModuleNode( const char* name ) { return name[0] == '@' || name[0] == '+'; }
static inline bool isSysvarNode( const char* name ) { return name[0] == '~'; }
static inline bool isCallNode  ( const char* name ) { return name[0] == '\''; }
static inline bool isResultNode( const char* name ) { return name[0] == '"'; }
static inline bool isVarNode   ( const char* name ) { return !(isModuleNode(name) || isSysvarNode(name) || isCallNode(name) || isResultNode(name)); }

#ifdef with_WrapCall

void addNodeDynamic( HWND hwndTree, HTREEITEM hParent );
void CallTree_RemoveDependResult( HWND hwndTree, HTREEITEM hItem );

void AddCallNode ( HWND hwndTree, const ModcmdCallInfo& callinfo );
void RemoveCallNode( HWND hwndTree, const ModcmdCallInfo& callinfo );
void AddResultNode( HWND hwndTree, const ResultNodeData* pResult );
void RemoveResultNode( HWND hwndTree, HTREEITEM hResult );

}

#endif
