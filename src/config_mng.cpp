
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"
#include "encoding.h"
#include "module\/supio\/supio.h"

std::unique_ptr<KnowbugConfig> g_knowbug_config;

KnowbugConfig::SingletonAccessor g_config;

static auto get_hsp_dir() -> OsString {
	// knowbug の DLL の絶対パスを取得する。
	TCHAR path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(nullptr), path, MAX_PATH);
	auto full_path = OsString{ path };

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

static auto ini_file_path(OsStringView const& hsp_dir) -> OsString {
	return OsString{ to_owned(hsp_dir) + TEXT("knowbug.ini") };
}

void KnowbugConfig::initialize() {
	assert(g_knowbug_config == nullptr);
	g_knowbug_config = std::make_unique<KnowbugConfig>(load(get_hsp_dir()));
}

auto KnowbugConfig::load(OsString&& hsp_dir) -> KnowbugConfig {
	auto&& ini = CIni{ ini_file_path(as_view(hsp_dir)) };
	auto config = KnowbugConfig{};

	config.hspDir = std::move(hsp_dir);
	config.bTopMost   = ini.getBool( "Window", "bTopMost", false );
	config.viewPosXIsDefault = !ini.existsKey("Window", "viewPosX");
	config.viewPosYIsDefault = !ini.existsKey("Window", "viewPosY");
	config.viewPosX   = ini.getInt("Window", "viewPosX", 0);
	config.viewPosY   = ini.getInt("Window", "viewPosY", 0);
	config.viewSizeX  = ini.getInt("Window", "viewSizeX", 412);
	config.viewSizeY  = ini.getInt("Window", "viewSizeY", 380);
	config.tabwidth   = ini.getInt( "Interface", "tabwidth", 3 );
	config.fontFamily = to_owned(ini.getString("Interface", "fontFamily", "MS Gothic"));
	config.fontSize   = ini.getInt("Interface", "fontSize", 13);
	config.fontAntialias = ini.getBool("Interface", "fontAntialias", false);

	config.logPath = to_owned(ini.getString("Log", "autoSavePath", ""));
	config.warnsBeforeClearingLog = ini.getBool("Log", "warnsBeforeClearingLog", true);
	config.scrollsLogAutomatically = ini.getBool("Log", "scrollsLogAutomatically", true);

	return config;
}

auto KnowbugConfig::selfPath() const -> OsString {
	return ini_file_path(as_view(hspDir));
}
