
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"

#include "module\/supio\/supio.h"

KnowbugConfig::SingletonAccessor g_config;

static auto SelfDir() -> string
{
	HSPAPICHAR path[MAX_PATH];
	char *hctmp1 = 0;
	GetModuleFileName(GetModuleHandle(nullptr), path, MAX_PATH);

	char drive[5];
	char dir[MAX_PATH];
	char _dummy[MAX_PATH];
	_splitpath_s(apichartohspchar(path,&hctmp1), drive, dir, _dummy, _dummy);
	freehc(&hctmp1);
	return string(drive) + dir;
}

KnowbugConfig::KnowbugConfig()
{
	hspDir = SelfDir();
	auto&& ini = CIni { selfPath().c_str() };
	
	bTopMost   = ini.getBool( "Window", "bTopMost", false );
	viewPosXIsDefault = !ini.existsKey("Window", "viewPosX");
	viewPosYIsDefault = !ini.existsKey("Window", "viewPosY");
	viewPosX   = ini.getInt("Window", "viewPosX", 0);
	viewPosY   = ini.getInt("Window", "viewPosY", 0);
	viewSizeX  = ini.getInt("Window", "viewSizeX", 412);
	viewSizeY  = ini.getInt("Window", "viewSizeY", 380);
	tabwidth   = ini.getInt( "Interface", "tabwidth", 3 );
	fontFamily = ini.getString("Interface", "fontFamily", "MS Gothic");
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
}
