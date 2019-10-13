#include "pch.h"
#include "hsp_runtime.h"
#include "hsp_objects.h"
#include "hsp_object_tree.h"
#include "hsp_wrap_call.h"
#include "hsx.h"
#include "hsx_debug_segment.h"
#include "source_files.h"
#include "string_split.h"

namespace hsx = hsp_sdk_ext;

class WcDebuggerImpl
	: public WcDebugger
{
	HSP3DEBUG* debug_;

	SourceFileRepository& source_file_repository_;

public:
	WcDebuggerImpl(HSP3DEBUG* debug, SourceFileRepository& source_file_repository)
		: debug_(debug)
		, source_file_repository_(source_file_repository)
	{
	}

	auto current_file_id_opt() const -> std::optional<std::size_t> {
		auto&& file_ref_name_opt = hsx::debug_to_file_ref_name(debug_);
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
		hsx::debug_do_update_location(debug_);

		file_id_opt = current_file_id_opt();
		line_index = hsx::debug_to_line_index(debug_);
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

static void read_debug_segment(HspObjectsBuilder& builder, SourceFileResolver& resolver, HSPCTX const* ctx) {
	auto reader = hsx::DebugSegmentReader{ ctx };
	while (true) {
		auto&& item_opt = reader.next();
		if (!item_opt) {
			break;
		}

		switch (item_opt->kind()) {
		case hsx::DebugSegmentItemKind::SourceFile:
			resolver.add_file_ref_name(std::string{ item_opt->str() });
			continue;

		case hsx::DebugSegmentItemKind::VarName:
			builder.add_var_name(item_opt->str());
			continue;

		case hsx::DebugSegmentItemKind::LabelName:
			builder.add_label_name(item_opt->num(), item_opt->str(), ctx);
			continue;

		case hsx::DebugSegmentItemKind::ParamName:
			builder.add_param_name(item_opt->num(), item_opt->str(), ctx);
			continue;

		default:
			continue;
		}
	}
}

auto HspRuntime::create(HSP3DEBUG* debug, OsString&& common_path)->std::unique_ptr<HspRuntime> {
	auto builder = HspObjectsBuilder{};
	auto resolver = SourceFileResolver{};

	resolver.add_known_dir(std::move(common_path));
	read_debug_segment(builder, resolver, hsx::debug_to_context(debug));

	auto source_file_repository = std::make_unique<SourceFileRepository>(resolver.resolve());

	auto logger = std::unique_ptr<HspLogger>{ std::make_unique<HspLoggerImpl>() };
	auto scripts = std::unique_ptr<HspScripts>{ std::make_unique<HspScriptsImpl>(*source_file_repository) };

	auto objects = std::make_unique<HspObjects>(builder.finish(debug, *logger, *scripts, *source_file_repository));

	auto object_tree = HspObjectTree::create(*objects);
	auto wc_debugger = std::shared_ptr<WcDebugger>{ std::make_shared<WcDebuggerImpl>(debug, *source_file_repository) };

	return std::make_unique<HspRuntime>(
		HspRuntime{
			std::move(debug),
			std::move(source_file_repository),
			std::move(logger),
			std::move(scripts),
			std::move(objects),
			std::move(object_tree),
			std::move(wc_debugger)
		});
}

void HspRuntime::update_location() {
	hsx::debug_do_update_location(debug_);
}
