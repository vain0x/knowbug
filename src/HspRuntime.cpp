#include "hpiutil/dinfo.hpp"
#include "HspDebugApi.h"
#include "HspRuntime.h"
#include "HspObjects.h"
#include "HspObjectTree.h"
#include "SourceFileResolver.h"

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

class HspScriptsImpl
	: public HspScripts
{
	SourceFileResolver& source_file_resolver_;

	std::string empty_;
	std::unordered_map<std::string, std::shared_ptr<std::string>> scripts_;

public:
	HspScriptsImpl(SourceFileResolver& source_file_resolver)
		: source_file_resolver_(source_file_resolver)
		, empty_(u8"ファイルが見つかりません")
	{
	}

	auto content(char const* file_ref_name) -> std::string const& override {
		auto&& iter = scripts_.find(file_ref_name);
		if (iter != scripts_.end()) {
			return *iter->second;
		}

		auto file_ref_name_os_str = to_os(as_hsp(file_ref_name));

		auto&& content_opt = source_file_resolver_.find_script_content(as_view(file_ref_name_os_str));
		if (!content_opt) {
			return empty_;
		}

		scripts_.emplace(std::string{ file_ref_name }, std::make_shared<std::string>(to_hsp(*content_opt)));
		return *scripts_.at(file_ref_name);
	}
};

HspRuntime::HspRuntime(HspDebugApi&& api, SourceFileResolver& source_file_resolver)
	: api_(std::move(api))
	, logger_(std::make_unique<HspLoggerImpl>())
	, scripts_(std::make_unique<HspScriptsImpl>(source_file_resolver))
	, static_vars_(api_)
	, objects_(api_, *logger_, *scripts_, static_vars_, hpiutil::DInfo::instance())
	, object_tree_(HspObjectTree::create(objects_))
{
}
