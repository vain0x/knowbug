
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

template<typename T>
T loadVswFunc(CIni& ini, HMODULE hDll, char const* vtname, char const* rawName)
{
	static auto const stc_sec = "VardataString/UserdefTypes/Func";

	auto const funcName = ini.getString(stc_sec, strf("%s.%s", vtname, rawName).c_str());
	auto const f = (T)(GetProcAddress(hDll, funcName));
	if ( funcName[0] != '\0' && ! f ) {
		Knowbug::logmesWarning(strf("拡張型表示用の %s 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s\r\n",
			rawName, vtname, funcName).c_str());
	}
	return f;
}

KnowbugConfig::KnowbugConfig()
{
	hspDir = SelfDir();
	auto&& ini = CIni { selfPath().c_str() };
	
	bTopMost   = ini.getBool( "Window", "bTopMost", false );
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

	vswInfo.resize(HSPVAR_FLAG_MAX + ctx->hsphed->max_varhpi);

	// 拙作プラグイン拡張型表示
	for ( auto&& vsw2 : vswInfoForInternal() ) {
		tryRegisterVswInfo(vsw2.vtname
			, VswInfo { nullptr, vsw2.addVar, vsw2.addValue });
	}

	// 拡張型の変数データを文字列化する関数
	auto const& keys = ini.enumKeys("VardataString/UserdefTypes");
	for ( auto const& vtname : keys ) {
		auto const dllPath = ini.getString("VardataString/UserdefTypes", vtname.c_str());
		if ( auto hDll = module_handle_t { LoadLibrary(dllPath) } ) {
			auto const fReceive  = loadVswFunc<receiveVswMethods_t>(ini, hDll.get(), vtname.c_str(), "receiveVswMethods");
			auto const fAddVar   = loadVswFunc<addVarUserdef_t  >(ini, hDll.get(), vtname.c_str(), "addVar");
			auto const fAddValue = loadVswFunc<addValueUserdef_t>(ini, hDll.get(), vtname.c_str(), "addValue");

#ifdef _DEBUG
			Knowbug::logmes(strf("型 %s の拡張表示情報が読み込まれた。\r\nVswInfo { %d, %d, %d }\r\n",
				vtname,
				hDll.get() != nullptr, fAddVar != nullptr, fAddValue != nullptr
			).c_str());
#endif
			tryRegisterVswInfo(vtname
				, VswInfo { std::move(hDll), fAddVar, fAddValue });
			if ( fReceive ) {
				fReceive(knowbug_getVswMethods());
			}
		} else {
			Knowbug::logmesWarning(strf("拡張型表示用の Dll の読み込みに失敗した。\r\n型名：%s, パス：%s\r\n",
				vtname, dllPath).c_str());
		}
	}
}

bool KnowbugConfig::tryRegisterVswInfo(string const& vtname, VswInfo vswi)
{
	auto const hvp = hpiutil::tryFindHvp(vtname.c_str());
	if ( ! hvp ) return false;

	auto const vtflag = static_cast<vartype_t>(hvp->flag);
	assert(0 < vtflag && vtflag < vswInfo.size());
	vswInfo[vtflag] = std::move(vswi);
	return true;
}
