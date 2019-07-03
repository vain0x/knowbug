
#include <fstream>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "Logger.h"

VTNodeLog::VTNodeLog()
{}

auto VTNodeLog::content() const -> OsStringView
{
	static auto content = OsString{};
	return content;
}

void VTNodeLog::setLogObserver(weak_ptr<LogObserver> observer)
{
}
