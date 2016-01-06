
#include <fstream>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"

struct VTNodeLog::Impl
{
	string log_;
	weak_ptr<LogObserver> observer_;
};

VTNodeLog::VTNodeLog()
	: p_(new Impl {})
{}

VTNodeLog::~VTNodeLog()
{
	if ( !g_config->logPath.empty() ) {
		save(g_config->logPath.c_str());
	}
}

auto VTNodeLog::str() const -> string const&
{
	return p_->log_;
}

bool VTNodeLog::save(char const* filePath) const
{
	std::ofstream ofs { filePath };
	ofs.write(str().c_str(), str().size());
	return ofs.good();
}

void VTNodeLog::clear()
{
	p_->log_.clear();
}

void VTNodeLog::append(char const* addition)
{
	p_->log_ += addition;

	if ( auto obs = p_->observer_.lock() ) {
		obs->afterAppend(addition);
	}
}

void VTNodeLog::setLogObserver(weak_ptr<LogObserver> obs)
{
	p_->observer_ = obs;
}
