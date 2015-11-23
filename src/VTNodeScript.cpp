
#include "VarTreeNodeData.h"
#include "module/LineDelimitedString.h"

struct VTNodeScript::Impl
{};

VTNodeScript::VTNodeScript()
	: p_(new Impl {})
{}

auto VTNodeScript::parent() const -> shared_ptr<VTNodeData>
{
	return VTRoot::make_shared();
}
