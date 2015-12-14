
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "main.h"
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
#endif
#include "VarTreeNodeData.h"

namespace detail {
struct TvObserver;
struct LogObserver;
} // namespace detail

class VTView
{
public:
	VTView();
	~VTView();

	void update();
	void updateViewWindow();

	void saveCurrentViewCaret(int vcaret);

	LRESULT customDraw(LPNMTVCUSTOMDRAW pnmcd);
	auto getItemVarText(HTREEITEM hItem) const -> shared_ptr<string const>;
	auto tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData>;

	void selectNode(VTNodeData const&);

private:
	struct Impl;
	unique_ptr<Impl> p_;

	friend struct detail::TvObserver;
	friend struct detail::LogObserver;
};
