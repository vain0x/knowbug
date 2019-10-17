// 設定の読み込みと管理

#pragma once

#include <memory>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"

// knowbug のすべての設定。package/knowbug.ini を参照。
class KnowbugConfig {
public:
	OsString hsp_dir_;

	bool top_most_;
	int tab_width_;

	bool view_pos_x_is_default_;
	bool view_pos_y_is_default_;
	int view_pos_x_;
	int view_pos_y_;
	int view_size_x_;
	int view_size_y_;

	OsString font_family_;
	int font_size_;
	bool font_antialias_;

	OsString log_path_;

public:
	static auto create()->std::unique_ptr<KnowbugConfig>;

	auto common_dir() const -> OsString {
		return OsString{ hsp_dir_ + TEXT("common") };
	}

	auto config_path() const->OsString;
};
