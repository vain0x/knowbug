#pragma once

#include "hsp_objects.h"
#include "hsp_object_tree.h"

class SourceFileRepository;
class WcDebugger;

// FIXME: 名前がよくないので変える

// HSP 関連の操作をまとめるもの。
// HSP 側から取得できる情報を knowbug 用に加工したりキャッシュしたりする機能を持つ (予定)
class HspRuntime {
	HSP3DEBUG* debug_;
	std::unique_ptr<SourceFileRepository> source_file_repository_;
	std::unique_ptr<HspLogger> logger_;
	std::unique_ptr<HspScripts> scripts_;
	std::unique_ptr<HspObjects> objects_;
	std::unique_ptr<HspObjectTree> object_tree_;
	std::shared_ptr<WcDebugger> wc_debugger_;

public:
	static auto create(HSP3DEBUG* debug, OsString&& common_path)->std::unique_ptr<HspRuntime>;

	HspRuntime(
		HSP3DEBUG* debug,
		std::unique_ptr<SourceFileRepository> source_file_repository,
		std::unique_ptr<HspLogger> logger,
		std::unique_ptr<HspScripts> scripts,
		std::unique_ptr<HspObjects> objects,
		std::unique_ptr<HspObjectTree> object_tree,
		std::shared_ptr<WcDebugger> wc_debugger
	)
		: debug_(debug)
		, source_file_repository_(std::move(source_file_repository))
		, logger_(std::move(logger))
		, scripts_(std::move(scripts))
		, objects_(std::move(objects))
		, object_tree_(std::move(object_tree))
		, wc_debugger_(std::move(wc_debugger))
	{
	}

	auto logger() -> HspLogger& {
		return *logger_;
	}

	auto objects() -> HspObjects& {
		return *objects_;
	}

	auto object_tree() -> HspObjectTree& {
		return *object_tree_;
	}

	auto wc_debugger() -> std::shared_ptr<WcDebugger> {
		return wc_debugger_;
	}

	void update_location();
};
