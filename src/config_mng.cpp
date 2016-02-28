
#include "module/strf.h"

#include "config_mng.h"
#include "ExVswInternal.h"

KnowbugConfig::SingletonAccessor g_config;

static auto SelfDir() -> string
{
	char path[MAX_PATH];
	GetModuleFileName(GetModuleHandle(nullptr), path, MAX_PATH);

	char drive[5];
	char dir[MAX_PATH];
	char _dummy[MAX_PATH];
	_splitpath_s(path, drive, dir, _dummy, _dummy);
	return string(drive) + dir;
}

template<typename T, typename TSec>
auto loadVswFunc(TSec&& sec, HMODULE hDll, char const* vtname, char const* rawName)
-> T
{
	auto iter = sec.find(strf("%s.%s", vtname, rawName));
	if ( iter == sec.end() ) return nullptr;
	auto const& funcName = iter->second;
	auto const f =
		reinterpret_cast<T>(GetProcAddress(hDll, funcName.c_str()));
	if ( ! f ) {
		Knowbug::logmesWarning
			(strf("拡張型表示用の %s 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s\r\n"
				, rawName, vtname, funcName).c_str());
	}
	return f;
}

KnowbugConfig::KnowbugConfig()
	: hspDir(SelfDir())
	, ini_(selfPath(), true)
{
	auto&& iniDefault = INI<> { hspDir + "knowbug_default.ini", true };
	ini_.merge(iniDefault);

	if ( enableCustomDraw() ) {

		//color of internal types
		for ( auto i = 0; i < HSPVAR_FLAG_USERDEF; ++i ) {
			clrText[i] = load<int>("textColor", strf("%d", i).c_str());
		}

		//color of external types or functions
		for ( auto const& kv : ini_["textColor"] ) {
			auto cref = static_cast<COLORREF>(std::atol(kv.second.c_str()));
			clrTextExtra.emplace(kv.first, cref);
		}
	}

	vswInfo.resize(HSPVAR_FLAG_MAX + ctx->hsphed->max_varhpi);

	// 拙作プラグイン拡張型表示
	for ( auto&& vsw2 : vswInfoForInternal() ) {
		tryRegisterVswInfo(vsw2.vtname
			, VswInfo { nullptr, vsw2.addVar, vsw2.addValue });
	}

	// 拡張型の変数データを文字列化する関数
	for ( auto const& kv : ini_["vsw"] ) {
		auto const vtname = kv.first.c_str();
		auto const& dllPath = kv.second;
		if ( auto hDll = module_handle_t { LoadLibrary(dllPath.c_str()) } ) {
			auto& sec = ini_["vswFunc"];
			auto const fReceive  = loadVswFunc<receiveVswMethods_t>(sec, hDll.get(), vtname, "receiveVswMethods");
			auto const fAddVar   = loadVswFunc<addVarUserdef_t    >(sec, hDll.get(), vtname, "addVar"           );
			auto const fAddValue = loadVswFunc<addValueUserdef_t  >(sec, hDll.get(), vtname, "addValue"         );

#ifdef _DEBUG
			Knowbug::logmes(
				strf("型 %s の拡張表示情報が読み込まれた。\r\nVswInfo { %d, %d, %d }\r\n"
					, vtname
					, !! hDll.get(), !! fAddVar, !! fAddValue
					).c_str());
#endif
			tryRegisterVswInfo(vtname
				, VswInfo { std::move(hDll), fAddVar, fAddValue });
			if ( fReceive ) {
				fReceive(knowbug_getVswMethods());
			}
		} else {
			Knowbug::logmesWarning(
				strf("拡張型表示用の Dll の読み込みに失敗した。\r\n型名：%s, パス：%s\r\n"
					, vtname, dllPath
					).c_str());
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
