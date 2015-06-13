
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#include "module/strf.h"

#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
# include "WrapCall/ResultNodeData.h"
using WrapCall::ModcmdCallInfo;
using WrapCall::ResultNodeData;
#endif
class CStaticVarTree;

namespace VarTree
{

void init();
void term();

LRESULT   customDraw(LPNMTVCUSTOMDRAW pnmcd);
vartype_t getVartype(HTREEITEM hItem );
std::shared_ptr<string const> getItemVarText(HTREEITEM hItem);

HTREEITEM getScriptNodeHandle();
HTREEITEM getLogNodeHandle();

// ツリービューのノードに関するメタ定義
enum class SystemNodeId {
#ifdef with_WrapCall
	Dynamic,
#endif
	Sysvar,
	Log,
	Script,
	General,
};
namespace Detail
{
	template<char Prefix, typename LParamType>
	struct NodeTag {
		using self_t = NodeTag<Prefix, LParamType>;
		using lparam_t = LParamType;
		static bool isTypeOf(char const* itemString) {
			return itemString[0] == Prefix;
		}
	};
	template<typename Tag> struct Verify {
		static bool apply(char const*, typename Tag::lparam_t const& value);
	};
}
struct ModuleNode : public Detail::NodeTag<'@', CStaticVarTree const*> { };
struct SystemNode : public Detail::NodeTag<'+', SystemNodeId> { };
struct CustomNode : public Detail::NodeTag<'*', label_t> { };
struct SysvarNode : public Detail::NodeTag<'~', Sysvar::Id> { }; 
struct InvokeNode : public Detail::NodeTag<'\'', int> { };
struct ResultNode : public Detail::NodeTag<'"', ResultNodeData*> { };
struct VarNode    : public Detail::NodeTag<'\0', PVal*> {
	static bool isTypeOf(char const * s);
};
template<typename Tag, typename lparam_t = typename Tag::lparam_t>
lparam_t TreeView_MyLParam(HWND hTree, HTREEITEM hItem, Tag* = nullptr);

#ifdef with_WrapCall

void AddCallNode(ModcmdCallInfo const& callinfo);
void RemoveLastCallNode();
void AddResultNode(ModcmdCallInfo const& callinfo, std::shared_ptr<ResultNodeData> pResult);
void RemoveResultNode( HTREEITEM hResult );
void UpdateCallNode();

#endif

HTREEITEM AddCustomNode(char const* name, char const* parentName, label_t lb);
bool RemoveCustomNode(HTREEITEM hItem);

}
