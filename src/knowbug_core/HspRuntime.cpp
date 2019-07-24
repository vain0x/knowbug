#include "pch.h"
#include "../hpiutil/dinfo.hpp"
#include "HspDebugApi.h"
#include "HspRuntime.h"
#include "HspObjects.h"
#include "HspObjectTree.h"
#include "hsp_wrap_call.h"
#include "SourceFileResolver.h"

class WcDebuggerImpl
	: public WcDebugger
{
	HspDebugApi& api_;

public:
	WcDebuggerImpl(HspDebugApi& api)
		: api_(api)
	{
	}

	void get_current_location(std::string& file_ref_name, std::size_t& line_index) override {
		api_.debug()->dbg_curinf();

		file_ref_name = api_.current_file_ref_name().value_or(u8"???");
		line_index = api_.current_line();
	}
};

class HspLoggerImpl
	: public HspLogger
{
	Utf8String content_;

public:
	auto content() const -> Utf8StringView override {
		return as_view(content_);
	}

	void append(Utf8StringView const& text) override {
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

	Utf8String empty_;
	std::unordered_map<std::string, std::shared_ptr<Utf8String>> scripts_;

public:
	HspScriptsImpl(SourceFileResolver& source_file_resolver)
		: source_file_resolver_(source_file_resolver)
		, empty_(to_owned(as_utf8(u8"ファイルが見つかりません")))
	{
	}

	auto content(char const* file_ref_name) -> Utf8StringView override {
		auto&& iter = scripts_.find(file_ref_name);
		if (iter != scripts_.end()) {
			return *iter->second;
		}

		auto file_ref_name_os_str = to_os(as_hsp(file_ref_name));

		auto&& content_opt = source_file_resolver_.find_script_content(as_view(file_ref_name_os_str));
		if (!content_opt) {
			return empty_;
		}

		scripts_.emplace(std::string{ file_ref_name }, std::make_shared<Utf8String>(to_utf8(*content_opt)));
		return *scripts_.at(file_ref_name);
	}

	auto line(char const* file_ref_name, std::size_t line_index) -> std::optional<Utf8String> override {
		auto&& iter = scripts_.find(file_ref_name);
		if (iter != scripts_.end()) {
			return *iter->second;
		}

		auto file_ref_name_os_str = to_os(as_hsp(file_ref_name));

		auto&& line_opt = source_file_resolver_.find_script_line(file_ref_name_os_str, line_index);
		if (!line_opt) {
			return std::nullopt;
		}

		return std::make_optional(to_utf8(*line_opt));
	}
};

HspRuntime::HspRuntime(HspDebugApi&& api, DebugInfo const& debug_info, SourceFileResolver& source_file_resolver)
	: api_(std::move(api))
	, logger_(std::make_unique<HspLoggerImpl>())
	, scripts_(std::make_unique<HspScriptsImpl>(source_file_resolver))
	, static_vars_(api_)
	, objects_(api_, *logger_, *scripts_, static_vars_, debug_info, hpiutil::DInfo::instance())
	, object_tree_(HspObjectTree::create(objects_))
	, wc_debugger_(std::make_shared<WcDebuggerImpl>(api_))
{
}
