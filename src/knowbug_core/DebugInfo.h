
#pragma once

#include <string>

#undef max

// FIXME: たいして役に立っていないので削除したい。HspObjects か HspDebugApi に統合したい
// HSP3DEBUG wrapper
class DebugInfo
{
	using string = std::string;

	HSP3DEBUG* const debug_;

public:
	DebugInfo(HSP3DEBUG* debug);
	~DebugInfo();

	bool setStepMode(int mode) { return (debug_->dbg_set(mode) >= 0); }

	auto fetchGeneralInfo() const -> std::vector<std::pair<string, string>>;
};
