#include "hpiutil/dinfo.hpp"
#include "HspDebugApi.h"
#include "HspRuntime.h"
#include "HspObjects.h"
#include "HspObjectTree.h"

class HspLoggerImpl
	: public HspLogger
{
	std::string content_;

public:
	auto content() const -> std::string const& override {
		return content_;
	}

	void append(char const* text) override {
		content_ += text;
	}

	void clear() override {
		content_.clear();
	}
};

HspRuntime::HspRuntime(HspDebugApi&& api)
	: api_(std::move(api))
	, logger_(std::make_unique<HspLoggerImpl>())
	, static_vars_(api_)
	, objects_(api_, *logger_, static_vars_, hpiutil::DInfo::instance())
	, object_tree_(HspObjectTree::create(objects_))
{
}
