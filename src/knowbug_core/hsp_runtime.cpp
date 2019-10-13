#include "pch.h"
#include "hsp_runtime.h"
#include "hsp_objects.h"
#include "hsp_object_tree.h"
#include "hsp_wrap_call.h"
#include "hsx.h"
#include "hsx_debug_segment.h"
#include "source_files.h"
#include "string_split.h"


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

	auto objects = std::make_unique<HspObjects>(builder.finish(debug, *logger, std::move(source_file_repository)));

	auto object_tree = HspObjectTree::create(*objects);

	return std::make_unique<HspRuntime>(
		HspRuntime{
			std::move(debug),
			std::move(logger),
			std::move(objects),
			std::move(object_tree)
		});
}

void HspRuntime::update_location() {
	hsx::debug_do_update_location(debug_);
}
