#include "pch.h"
#include "../hpiutil/dinfo.hpp"
#include "HspDebugApi.h"
#include "HspRuntime.h"
#include "HspObjects.h"
#include "HspObjectTree.h"
#include "hsp_wrap_call.h"
#include "source_files.h"

namespace hsx = hsp_sdk_ext;

class WcDebuggerImpl
	: public WcDebugger
{
	HspDebugApi& api_;

	SourceFileRepository& source_file_repository_;

public:
	WcDebuggerImpl(HspDebugApi& api, SourceFileRepository& source_file_repository)
		: api_(api)
		, source_file_repository_(source_file_repository)
	{
	}

	auto current_file_id_opt() const -> std::optional<std::size_t> {
		auto&& file_ref_name_opt = api_.current_file_ref_name();
		if (!file_ref_name_opt) {
			return std::nullopt;
		}

		auto&& id_opt = source_file_repository_.file_ref_name_to_file_id(*file_ref_name_opt);
		if (!id_opt) {
			return std::nullopt;
		}

		return id_opt->id();
	}

	void get_current_location(std::optional<std::size_t>& file_id_opt, std::size_t& line_index) override {
		hsx::debug_do_update_location(api_.debug());

		file_id_opt = current_file_id_opt();
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
	SourceFileRepository& source_file_repository_;

	Utf8String empty_;

public:
	HspScriptsImpl(SourceFileRepository& source_file_repository)
		: source_file_repository_(source_file_repository)
		, empty_(to_owned(as_utf8(u8"ファイルが見つかりません")))
	{
	}

	auto content(char const* file_ref_name) -> Utf8StringView override {
		return source_file_repository_.file_ref_name_to_content(file_ref_name).value_or(empty_);
	}

	auto line(char const* file_ref_name, std::size_t line_index) -> std::optional<Utf8StringView> override {
		return source_file_repository_.file_ref_name_to_line_at(file_ref_name, line_index);
	}
};

HspRuntime::HspRuntime(HspDebugApi&& api, DebugInfo const& debug_info, SourceFileRepository& source_file_repository)
	: api_(std::move(api))
	, logger_(std::make_unique<HspLoggerImpl>())
	, scripts_(std::make_unique<HspScriptsImpl>(source_file_repository))
	, static_vars_(api_)
	, objects_(api_, *logger_, *scripts_, static_vars_, debug_info, hpiutil::DInfo::instance(), source_file_repository)
	, object_tree_(HspObjectTree::create(objects_))
	, wc_debugger_(std::make_shared<WcDebuggerImpl>(api_, source_file_repository))
{
}

void HspRuntime::update_location() {
	hsx::debug_do_update_location(api_.debug());
}
