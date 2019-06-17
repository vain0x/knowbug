
#include <fstream>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"
#include "Logger.h"

VTNodeLog::VTNodeLog(std::shared_ptr<Logger> logger)
	: logger_(logger)
{}

auto VTNodeLog::content() const -> OsStringView
{
	return logger_->content();
}

void VTNodeLog::setLogObserver(weak_ptr<LogObserver> observer)
{
	logger_->set_observer(observer);
}
