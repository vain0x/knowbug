
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"
#include "encoding.h"
#include "module\/supio\/supio.h"

KnowbugConfig::SingletonAccessor g_config;

static auto SelfDir() -> OsString {
	// knowbug の DLL の絶対パスを取得する。
	HSPAPICHAR path[MAX_PATH];
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

	auto ini_file_path = selfPath().to_hsp_string();
	auto&& ini = CIni{ ini_file_path.data() };
	
	bTopMost   = ini.getBool( "Window", "bTopMost", false );
	viewPosXIsDefault = !ini.existsKey("Window", "viewPosX");
	viewPosYIsDefault = !ini.existsKey("Window", "viewPosY");
	viewPosX   = ini.getInt("Window", "viewPosX", 0);
	viewPosY   = ini.getInt("Window", "viewPosY", 0);
	viewSizeX  = ini.getInt("Window", "viewSizeX", 412);
	viewSizeY  = ini.getInt("Window", "viewSizeY", 380);
	tabwidth   = ini.getInt( "Interface", "tabwidth", 3 );
	fontFamily = SjisStringView{ ini.getString("Interface", "fontFamily", "MS Gothic") }.to_os_string();
	fontSize   = ini.getInt("Interface", "fontSize", 13);
	fontAntialias = ini.getBool("Interface", "fontAntialias", false);

	maxLength    = ini.getInt("Varinfo", "maxlen", 0x10000 - 1);
	infiniteNest = ini.getInt("Varinfo", "infiniteNest", 8);
	showsVariableAddress = ini.getBool("Varinfo", "showsVariableAddress", true);
	showsVariableSize    = ini.getBool("Varinfo", "showsVariableSize", true);
	showsVariableDump    = ini.getBool("Varinfo", "showsVariableDump", true);
	cachesVardataString  = ini.getBool("Varinfo", "cachesVardataString", false);
	prefixHiddenModule   = ini.getString("Varinfo", "prefixHiddenModule", "@__");
	
	bResultNode = ini.getBool( "Varinfo", "useResultNode", false );
	bCustomDraw = ini.getBool( "ColorType", "bCustomDraw", false );

	logPath = ini.getString("Log", "autoSavePath", "");
	warnsBeforeClearingLog = ini.getBool("Log", "warnsBeforeClearingLog", true);
	scrollsLogAutomatically = ini.getBool("Log", "scrollsLogAutomatically", true);
#ifdef with_WrapCall
	logsInvocation = ini.getBool("Log", "logsInvocation", false);
#endif

#if 0
	// FIXME: 一時的に廃止
	if ( bCustomDraw ) {
		//color of internal types
		for ( auto i = 0; i < HSPVAR_FLAG_USERDEF; ++i ) {
			clrText[i] = ini.getInt("ColorType", strf("text#%d", i).c_str(), RGB(0, 0, 0));
		}

		//color of external types or functions
		auto const& keys = ini.enumKeys("ColorTypeExtra");
		for ( auto const& key : keys ) {
			auto const cref = static_cast<COLORREF>(ini.getInt("ColorTypeExtra", key.c_str()));
			clrTextExtra.emplace(key, cref);
		}
	}
#endif
}
