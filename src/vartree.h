
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"

#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
# include "VarTreeNodeData.h"
#endif
class StaticVarTree;

namespace VarTree {

void init();
void term();

LRESULT customDraw(LPNMTVCUSTOMDRAW pnmcd);
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
struct ModuleNode : public Detail::NodeTag<'@', StaticVarTree const*> { };
struct SystemNode : public Detail::NodeTag<'+', SystemNodeId> { };
struct SysvarNode : public Detail::NodeTag<'~', hpiutil::Sysvar::Id> { };
#ifdef with_WrapCall
struct InvokeNode : public Detail::NodeTag<'\'', int> { };
struct ResultNode : public Detail::NodeTag<'"', ResultNodeData*> { };
#endif //defined(with_WrapCall)
struct VarNode    : public Detail::NodeTag<'\0', PVal*> {
	static bool isTypeOf(char const * s);
};
template<typename Tag, typename lparam_t = typename Tag::lparam_t>
lparam_t TreeView_MyLParam(HWND hTree, HTREEITEM hItem, Tag* = nullptr);

#ifdef with_WrapCall

void AddCallNode(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo);
void RemoveLastCallNode();
void AddResultNode(WrapCall::ModcmdCallInfo::shared_ptr_type const& callinfo, shared_ptr<ResultNodeData const> const& pResult);
void RemoveResultNode( HTREEITEM hResult );
void UpdateCallNode();

#endif

} //namespace VarTree
