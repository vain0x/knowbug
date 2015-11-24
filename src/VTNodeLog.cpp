
#include <fstream>
#include "main.h"
#include "VarTreeNodeData.h"
#include "config_mng.h"

struct VTNodeLog::Impl
{
	string log_;
	shared_ptr<LogObserver> observer_;
};

VTNodeLog::VTNodeLog()
	: p_(new Impl {})
{}

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

	if ( p_->observer_ ) {
		p_->observer_->afterAppend(addition);
	}
}

void VTNodeLog::setLogObserver(shared_ptr<LogObserver> obs)
{
	p_->observer_ = obs;
}
