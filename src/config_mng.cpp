
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"
#include "encoding.h"
#include "module\/supio\/supio.h"

KnowbugConfig::SingletonAccessor g_config;

static auto SelfDir() -> OsString {
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

KnowbugConfig::KnowbugConfig()
{
	hspDir = SelfDir();

	auto&& ini = CIni{ selfPath() };

	bTopMost   = ini.getBool( "Window", "bTopMost", false );
	viewPosXIsDefault = !ini.existsKey("Window", "viewPosX");
	viewPosYIsDefault = !ini.existsKey("Window", "viewPosY");
	viewPosX   = ini.getInt("Window", "viewPosX", 0);
	viewPosY   = ini.getInt("Window", "viewPosY", 0);
	viewSizeX  = ini.getInt("Window", "viewSizeX", 412);
	viewSizeY  = ini.getInt("Window", "viewSizeY", 380);
	tabwidth   = ini.getInt( "Interface", "tabwidth", 3 );
	fontFamily = ini.getString("Interface", "fontFamily", "MS Gothic").to_owned();
	fontSize   = ini.getInt("Interface", "fontSize", 13);
	fontAntialias = ini.getBool("Interface", "fontAntialias", false);

	maxLength    = ini.getInt("Varinfo", "maxlen", 0x10000 - 1);
	infiniteNest = ini.getInt("Varinfo", "infiniteNest", 8);
	showsVariableAddress = ini.getBool("Varinfo", "showsVariableAddress", true);
	showsVariableSize    = ini.getBool("Varinfo", "showsVariableSize", true);
	showsVariableDump    = ini.getBool("Varinfo", "showsVariableDump", true);
	cachesVardataString  = ini.getBool("Varinfo", "cachesVardataString", false);
	prefixHiddenModule   = ini.getString("Varinfo", "prefixHiddenModule", "@__").to_hsp_string();

	bResultNode = ini.getBool( "Varinfo", "useResultNode", false );

	logPath = ini.getString("Log", "autoSavePath", "").to_owned();
	warnsBeforeClearingLog = ini.getBool("Log", "warnsBeforeClearingLog", true);
	scrollsLogAutomatically = ini.getBool("Log", "scrollsLogAutomatically", true);
#ifdef with_WrapCall
	logsInvocation = ini.getBool("Log", "logsInvocation", false);
#endif
}
