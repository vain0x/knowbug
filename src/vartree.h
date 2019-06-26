
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "hpiutil/dinfo.hpp"
#include "main.h"
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
#endif
#include "VarTreeNodeData.h"

class HspObjects;
class HspObjectTree;

namespace detail {
struct TvObserver;
struct VarTreeLogObserver;
} // namespace detail

class AbstractViewBox {
public:
	virtual ~AbstractViewBox() {
	}

	virtual auto current_scroll_line() const -> std::size_t = 0;
};

class VTView
{
public:
	VTView(hpiutil::DInfo const& debug_segment, HspObjects& objects_, HspStaticVars& static_vars, HspObjectTree& object_tree, HWND tv_handle_);
	~VTView();

	void update();
	void updateViewWindow(AbstractViewBox& view_box);

	void saveCurrentViewCaret(int vcaret);

	auto getItemVarText(HTREEITEM hItem) const -> unique_ptr<OsString>;
	auto tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData>;

	void selectNode(VTNodeData const&);

private:
	struct Impl;
	unique_ptr<Impl> p_;

	hpiutil::DInfo const& debug_segment_;
	HspObjects& objects_;
	HspStaticVars& static_vars_;
	HspObjectTree& object_tree_;

	friend struct detail::TvObserver;
	friend struct detail::VarTreeLogObserver;
};
