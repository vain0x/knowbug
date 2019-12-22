#pragma once

#include "encoding.h"
#include "hsp_object_key_fwd.h"

class HspObjectService {
public:
	static auto create(HSP3DEBUG* debug)->std::unique_ptr<HspObjectService>;

	virtual ~HspObjectService() {
	}

	virtual auto module_to_name(HspObjectKey::Module const& m) const->std::optional<Utf8StringView> = 0;
};
