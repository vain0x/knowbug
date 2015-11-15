
#include <windows.h>
#include <CommCtrl.h>
#include <stack>
#include <vector>
#include <map>
#include <cstring>
#include "../module/ptr_cast.h"
#include "../module/strf.h"
#include "../module/GuiUtility.h"
#include "TvRepr.h"

static TVINSERTSTRUCT& getTvIs(HTREEITEM hParent);

namespace DataTree {

struct TvRepr::Impl
{
	HWND hTv_;

	using mapNodeToTvItem_t = std::map<tree_t, HTREEITEM>;
	mapNodeToTvItem_t itemFromNode_;

	TvAppendObserver appendObserver;
	TvRemoveObserver removeObserver;
public:
	Impl(HWND hTv, TvAppendObserver _append, TvRemoveObserver _remove)
		: hTv_(hTv), appendObserver(_append), removeObserver(_remove)
	{ }
};

TvRepr::TvRepr(HWND hTv)
	: m(new Impl(hTv, TvAppendObserver(*this), TvRemoveObserver(*this)))
{
	using namespace std::placeholders;
	auto&& spawnRoot = std::bind(&TvRepr::spawnRoot, this, _1);
	registerObserver({ std::move(spawnRoot), &m->appendObserver, &m->removeObserver });
}

TvRepr::~TvRepr()
{}

void TvRepr::spawnRoot(NodeGlobal* root)
{
	insertItem(root, root->getName(), TVI_ROOT, TVI_LAST);
}
	
void TvRepr::TvAppendObserver::visit0(tree_t newChild)
{
	auto const parent = newChild->getParent();
	assert(parent);
	getCallback().insertItem
		( newChild
		, newChild->getName()
		, getCallback().tryFindTvItem(parent)
		, TVI_LAST);
}

void TvRepr::TvRemoveObserver::visit0(tree_t removed)
{
	auto const hItem = getCallback().tryFindTvItem(removed);
	assert(hItem);
	TreeView_DeleteItem(getCallback().m->hTv_, hItem);

	getCallback().m->itemFromNode_.erase(removed);
}

HTREEITEM TvRepr::tryFindTvItem(tree_t node) const
{
	auto const iter = m->itemFromNode_.find(node);
	return (iter != m->itemFromNode_.end() ? iter->second : nullptr);
}

tree_t TvRepr::tryFindNode(HTREEITEM hItem) const
{
	return reinterpret_cast<tree_t>(TreeView_GetItemLParam(m->hTv_, hItem));
}

void TvRepr::insertItem(tree_t node, string name, HTREEITEM hParent, HTREEITEM hInsertAfter)
{
	TVINSERTSTRUCT tvis {};
	tvis.hParent      = hParent;
	tvis.hInsertAfter = hInsertAfter;
	tvis.item.mask    = TVIF_TEXT | TVIF_PARAM;
	tvis.item.lParam  = (LPARAM)(address_cast(node));
	tvis.item.pszText = const_cast<char*>(name.c_str());
	
	HTREEITEM const hNewItem = TreeView_InsertItem(m->hTv_, &tvis);
	m->itemFromNode_.emplace(node, hNewItem);
}

}
