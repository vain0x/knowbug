
#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include "hpiutil/dinfo.hpp"
#include "main.h"
#ifdef with_WrapCall
# include "WrapCall/ModcmdCallInfo.h"
#endif
#include "VarTreeNodeData.h"

namespace detail {
struct TvObserver;
struct VarTreeLogObserver;
} // namespace detail

class VTView
{
public:
	VTView(hpiutil::DInfo const& debug_segment, HspStaticVars& static_vars);
	~VTView();

	void update();
	void updateViewWindow();

	void saveCurrentViewCaret(int vcaret);

	auto getItemVarText(HTREEITEM hItem) const -> unique_ptr<OsString>;
	auto tryGetNodeData(HTREEITEM hItem) const -> optional_ref<VTNodeData>;

	void selectNode(VTNodeData const&);

private:
	struct Impl;
	unique_ptr<Impl> p_;

	hpiutil::DInfo const& debug_segment_;
	HspStaticVars& static_vars_;

	friend struct detail::TvObserver;
	friend struct detail::VarTreeLogObserver;
};
