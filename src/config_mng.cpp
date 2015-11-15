
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"
#include "ExVswInternal.h"

KnowbugConfig::SingletonAccessor g_config;

static string SelfDir() {
	char path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(nullptr), path, MAX_PATH);

	char drive[5];
	char dir[MAX_PATH];
	char _dummy[MAX_PATH];
	_splitpath_s(path, drive, dir, _dummy, _dummy);
	return string(drive) + dir;
}

KnowbugConfig::KnowbugConfig()
{
	hspDir = SelfDir();
	CIni ini { selfPath().c_str() };

	bTopMost   = ini.getBool( "Window", "bTopMost", false );
	viewSizeX  = ini.getInt("Window", "viewSizeX", 412);
	viewSizeY  = ini.getInt("Window", "viewSizeY", 380);
	tabwidth   = ini.getInt( "Interface", "tabwidth", 3 );

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
		for ( int i = 0; i < HSPVAR_FLAG_USERDEF; ++i ) {
			clrText[i] = ini.getInt("ColorType", strf("text#%d", i).c_str(), RGB(0, 0, 0));
		}

		//color of external types or functions
		auto const& keys = ini.enumKeys("ColorTypeExtra");
		for ( auto const& key : keys ) {
			COLORREF const cref = ini.getInt("ColorTypeExtra", key.c_str());
			clrTextExtra.emplace(key, cref);
		}
	}

	// 拡張型の変数データを文字列化する関数
	auto const& keys = ini.enumKeys("VardataString/UserdefTypes");
	for ( auto const& vtname : keys ) {
		auto const dllPath = ini.getString("VardataString/UserdefTypes", vtname.c_str());
		if ( module_handle_t hDll { LoadLibrary(dllPath) } ) {
			static char const* const stc_sec = "VardataString/UserdefTypes/Func";

			auto const fnameAddVar = ini.getString(stc_sec, strf("%s.addVar", vtname).c_str());
			auto const fAddVar = (addVarUserdef_t)GetProcAddress(hDll.get(), fnameAddVar);
			if ( fnameAddVar[0] != '\0' && !fAddVar ) {
				Knowbug::logmesWarning(strf("拡張型表示用の addVar 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s\r\n",
					vtname, fnameAddVar).c_str());
			}

			auto const fnameAddValue = ini.getString(stc_sec, strf("%s.addValue", vtname).c_str());
			auto const fAddValue = (addValueUserdef_t)GetProcAddress(hDll.get(), fnameAddValue);
			if ( fnameAddValue[0] != '\0' && !fAddValue ) {
				Knowbug::logmesWarning(strf("拡張型表示用の addValue 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s\r\n",
					vtname, fnameAddValue).c_str());
			}

#ifdef _DEBUG
			Knowbug::logmes(strf("型 %s の拡張表示情報が読み込まれた。\r\nVswInfo { %d, %d, %d }\r\n",
				vtname,
				hDll.get() != nullptr, fAddVar != nullptr, fAddValue != nullptr
			).c_str());
#endif
			vswInfo.emplace(vtname, VswInfo { std::move(hDll), fAddVar, fAddValue });
		} else {
			Knowbug::logmesWarning(strf("拡張型表示用の Dll の読み込みに失敗した。\r\n型名：%s, パス：%s\r\n",
				vtname, dllPath).c_str());
		}
	}

	// 拙作プラグイン拡張型表示がなければ追加しておく
	for ( auto&& vsw2 : vswInfoForInternal() ) {
		//doesn't overwrite writers of external Dll
		map_find_or_insert(vswInfo, vsw2.vtname, [&vsw2]() {
			return VswInfo { nullptr, vsw2.addVar, vsw2.addValue };
		});
	}
}
