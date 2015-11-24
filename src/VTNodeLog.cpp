
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"

struct VTNodeLog::Impl
{};

VTNodeLog::VTNodeLog()
	: p_(new Impl {})
{}
