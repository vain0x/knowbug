#include "pch.h"
#include "hsx.h"
#include "hsx_debug_segment.h"
#include "hsp_object_key.h"
#include "hsp_object_service.h"
#include "hsp_objects_module_tree.h"

static constexpr auto GLOBAL_MODULE_ID = std::size_t{ 0 };

class HspObjectServiceImpl
	: public HspObjectService
{
	HSP3DEBUG* debug_;

	std::vector<Utf8String> var_names_;
	std::vector<Utf8String> module_names_;
	std::vector<std::vector<std::size_t>> module_vars_;

public:
	explicit HspObjectServiceImpl(HSP3DEBUG* debug)
		: debug_(debug)
		, var_names_()
		, module_names_()
		, module_vars_()
	{
		initialize();
	}

	auto module_global() const->HspObjectKey::Module {
		return HspObjectKey::Module{ GLOBAL_MODULE_ID };
	}

	auto module_count() const -> std::size_t override {
		assert(module_names_.size() == module_vars_.size());
		return module_names_.size();
	}

	auto module_to_name(HspObjectKey::Module const& m) const -> std::optional<Utf8StringView> override {
		if (m.module_id() >= module_names_.size()) {
			return std::nullopt;
		}

		return module_names_[m.module_id()];
	}

	auto module_to_static_var_count(HspObjectKey::Module const& m) const->std::size_t override {
		if (m.module_id() >= module_vars_.size()) {
			return 0;
		}

		return module_vars_[m.module_id()].size();
	}

	auto module_to_static_var_at(HspObjectKey::Module const& m, std::size_t index) const->std::optional<HspObjecKey::StaticVar> override {
		if (m.module_id() >= module_vars_.size() || index >= module_vars_[m.module_id()].size()) {
			return std::nullopt;
		}

		return HspObjectKey::StaticVar{ module_vars_[m.module_id][index] };
	}

private:
	auto debug() const -> HSP3DEBUG const* {
		return debug_;
	}

	auto debug() -> HSP3DEBUG* {
		return debug_;
	}

	// -------------------------------------------
	// 初期化
	// -------------------------------------------

	void initialize() {
		read_debug_segment();
		add_modules();
	}

	void read_debug_segment() {
		auto reader = hsx::DebugSegmentReader{ ctx };
		while (true) {
			auto item_opt = reader.next();
			if (!item_opt) {
				break;
			}

			switch (item_opt->kind()) {
			case hsx::DebugSegmentItemKind::SourceFile:
				continue;

			case hsx::DebugSegmentItemKind::VarName:
				var_names_.push_back(to_utf8(as_hsp(item_opt->str())));
				continue;

			case hsx::DebugSegmentItemKind::LabelName:
				continue;

			case hsx::DebugSegmentItemKind::ParamName:
				continue;

			default:
				continue;
			}
		}
	}

	void add_modules() {
		class ModuleTreeBuilder
			: public ModuleTreeListener
		{
			std::vector<Utf8String>& modules_;
			std::vector<std::vector<std::size_t>>& module_vars_;

		public:
			explicit ModuleTreeBuilder(std::vector<Utf8String>& modules, std::vector<std::vector<std::size_t>>& module_vars)
				: modules_(modules)
				, module_vars_(module_vars)
			{
			}

			void begin_module(Utf8StringView const& module_name) override {
				modules_.emplace_back(to_owned(module_name));
				module_vars_.emplace_back();
			}

			void end_module() override {
			}

			void add_var(std::size_t var_id, Utf8StringView const& var_name) override {
				if (modules_.empty()) {
					assert(false);
					return;
				}

				module_vars_.back().push_back(var_id);
			}
		};

		auto builder = ModuleTreeBuilder{ module_names_, module_vars_ };
		traverse_module_tree(var_names_, builder);

		assert(!module_names_.empty());
		assert(module_names_.size() == module_vars_.size());

		assert(module_names_[GLOBAL_MODULE_ID] == as_utf8(u8"@"));
	}
};

auto HspObjectService::create(HSP3DEBUG* debug)->std::unique_ptr<HspObjectService> {
	return std::make_unique<HspObjectServiceImpl>(debug);
}
