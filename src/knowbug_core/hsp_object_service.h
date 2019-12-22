#pragma once

#include "encoding.h"
#include "hsp_object_key_fwd.h"

class HspObjectService {
public:
	static auto create(HSP3DEBUG* debug)->std::unique_ptr<HspObjectService>;

	virtual ~HspObjectService() {
	}

	virtual auto module_global() const->HspObjectKey::Module = 0;

	virtual auto module_count() const->std::size_t = 0;

	virtual auto module_to_name(HspObjectKey::Module const& m) const->std::optional<Utf8StringView> = 0;

	virtual auto module_to_static_var_count(HspObjectKey::Module const& m) const->std::size_t = 0;

	virtual auto module_to_static_var_at(HspObjectKey::Module const& m, std::size_t index) const->std::optional<HspObjectKey::StaticVar> = 0;
};
