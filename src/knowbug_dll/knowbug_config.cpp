#include "pch.h"
#include <array>
#include "../knowbug_core/encoding.h"
#include "knowbug_config.h"
#include "win_ini.h"

static auto get_hsp_dir() -> OsString {
	// knowbug の DLL の絶対パスを取得する。
	auto buffer = std::array<TCHAR, MAX_PATH>{};
	GetModuleFileName(GetModuleHandle(nullptr), buffer.data(), buffer.size());
	auto full_path = OsString{ buffer.data() };

	// ファイル名の部分を削除
	while (!full_path.empty()) {
		auto last = full_path[full_path.length() - 1];
		if (last == TEXT('/') || last == TEXT('\\')) {
			break;
		}

		full_path.pop_back();
	}

	return full_path;
}

static auto get_config_path(OsStringView hsp_dir) -> OsString {
	return to_owned(hsp_dir) + TEXT("knowbug.ini");
}

static auto load_config(OsString&& hsp_dir) -> KnowbugConfig {
	auto&& ini = IniFile{ get_config_path(hsp_dir) };
	auto config = KnowbugConfig{};

	config.hsp_dir_ = std::move(hsp_dir);
	config.log_path_ = ini.get_string("Log", "autoSavePath", "");

	config.top_most_ = ini.get_bool("Window", "bTopMost", false);
	config.tab_width_ = ini.get_int("Interface", "tabwidth", 3);

	config.view_pos_x_is_default_ = !ini.contains_key("Window", "viewPosX");
	config.view_pos_x_ = ini.get_int("Window", "viewPosX", 0);

	config.view_pos_y_is_default_ = !ini.contains_key("Window", "viewPosY");
	config.view_pos_y_ = ini.get_int("Window", "viewPosY", 0);

	config.view_size_x_ = ini.get_int("Window", "viewSizeX", 412);
	config.view_size_y_ = ini.get_int("Window", "viewSizeY", 380);

	config.font_family_ = ini.get_string("Interface", "fontFamily", "MS Gothic");
	config.font_size_ = ini.get_int("Interface", "fontSize", 13);
	config.font_antialias_ = ini.get_bool("Interface", "fontAntialias", false);
	return config;
}

auto KnowbugConfig::create() -> std::unique_ptr<KnowbugConfig> {
	return std::make_unique<KnowbugConfig>(load_config(get_hsp_dir()));
}

auto KnowbugConfig::config_path() const -> OsString {
	return get_config_path(hsp_dir_);
}
