
#include "module/CIni.h"
#include "module/strf.h"

#include "config_mng.h"

std::unique_ptr<KnowbugConfig> KnowbugConfig::instance_;
KnowbugConfig::Mng g_config;

KnowbugConfig::KnowbugConfig()
{
	char ownpath[MAX_PATH];
	{
		GetModuleFileName( GetModuleHandle(nullptr), ownpath, MAX_PATH );
		
		char drive[5];
		char dir[MAX_PATH];
		char _dummy[MAX_PATH];		// ダミー
		
		_splitpath_s( ownpath, drive, dir, _dummy, _dummy );
		sprintf_s( ownpath, "%s%s", drive, dir );
	}
	
	string const ownpath_full = strf("%sknowbug.ini", ownpath);
	CIni ini { ownpath_full.c_str() };
	
	// common フォルダへのパス
	commonPath = strf( "%scommon", ownpath );
	
	// 最大表示データ量
	//maxlenVarinfo = ini.getInt( "Varinfo", "maxlen", 0x1000 - 1 );
	logMaxlen     = ini.getInt( "Log",     "maxlen", 0x20000 );
	
	// タブ文字幅
	tabwidth  = ini.getInt( "Interface", "tabwidth", 3 );
	
	// 右端で折り返すか否か
	//bWordwrap = ini.getBool( "Interface", "bWordwrap", false );
	
	// 最前面ウィンドウか否か
	bTopMost = ini.getBool( "Window", "bTopMost", false );
	
	// 自動保存パス
	logPath = ini.getString( "Log", "autoSavePath", "" );
	
	// 返値ノードを使うか
	bResultNode = ini.getBool( "Varinfo", "useResultNode", false );

	// カスタムドローするかどうか
	bCustomDraw = ini.getBool( "ColorType", "bCustomDraw", false );
	
	if ( bCustomDraw ) {
		// 型ごとの色
		for ( int i = 0; i < HSPVAR_FLAG_USERDEF; ++i ) {
			clrText[i] = ini.getInt("ColorType", strf("text#%d", i).c_str(), RGB(0, 0, 0));
		}

		// 呼び出しや拡張型の色
		auto const keys = ini.enumKeys("ColorTypeExtra");
		for ( auto const& key : keys ) {
			COLORREF const cref = ini.getInt("ColorTypeExtra", key.c_str());
			//dbgout("extracolor: %s = (%d, %d, %d)", key.c_str(), GetRValue(cref), GetGValue(cref), GetBValue(cref) );
			clrTextExtra.insert({ key, cref });
		}
	}

	// 拡張型の変数データを文字列化する関数
	auto keys = ini.enumKeys("VardataString/UserdefTypes");
	//dbgout(join(keys.begin(), keys.end(), ", ").c_str());

	for ( auto const& vtname : keys ) {
		auto const dllPath = ini.getString("VardataString/UserdefTypes", vtname.c_str());
		if ( auto const hDll = LoadLibrary(dllPath) ) {
			static char const* const stc_sec = "VardataString/UserdefTypes/Func";

			auto const fnameAddVar = ini.getString(stc_sec, strf("%s.addVar", vtname.c_str()).c_str());
			auto const fAddVar = (addVarUserdef_t)GetProcAddress(hDll, fnameAddVar);
			if ( fnameAddVar[0] != '\0' && !fAddVar ) {
				Knowbug::logmesWarning(strf("拡張型表示用の addVar 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s",
					vtname.c_str(), fnameAddVar).c_str());
			}

			auto const fnameAddValue = ini.getString(stc_sec, strf("%s.addValue", vtname.c_str()).c_str());
			auto const fAddValue = (addValueUserdef_t)GetProcAddress(hDll, fnameAddValue);
			if ( fnameAddValue[0] != '\0' && !fAddValue ) {
				Knowbug::logmesWarning(strf("拡張型表示用の addValue 関数が読み込まれなかった。\r\n型名：%s, 関数名：%s",
					vtname.c_str(), fnameAddValue).c_str());
			}

#ifdef _DEBUG
			Knowbug::logmes(strf("型 %s の拡張表示情報が読み込まれた。\r\nVswInfo { %08X, %08X, %08X }",
				vtname.c_str(), hDll, fAddVar, fAddValue).c_str());
#endif
			vswInfo.insert({ vtname, VswInfo { hDll, fAddVar, fAddValue } });
		} else {
			Knowbug::logmesWarning(strf("拡張型表示用の Dll の読み込みに失敗した。\r\n型名：%s, パス：%s",
				vtname.c_str(), dllPath).c_str());
		}
	}

	// 拙作プラグイン拡張型表示がなければ追加しておく
	struct VswInfoForInternal { string vtname; addVarUserdef_t addVar; addValueUserdef_t addValue; };
	static VswInfoForInternal const stc_vswInfoForInternal[] = {
#ifdef with_Assoc
		{ "assoc_k", nullptr, knowbugVsw_addValueAssoc },
#endif
#ifdef with_Vector
		{ "vector_k", knowbugVsw_addVarVector, knowbugVsw_addValueVector },
#endif
#ifdef with_Array
		{ "array_k", knowbugVsw_addVarArray, knowbugVsw_addValueArray },
#endif
#ifdef with_Modcmd
		{ "modcmd_k", nullptr, knowbugVsw_addValueModcmd },
#endif
	};
	for ( auto vsw2 : stc_vswInfoForInternal ) {
		// ini ファイルの指定を優先する
		if ( vswInfo.find(vsw2.vtname) == vswInfo.end() ) {
			vswInfo.insert({ vsw2.vtname, VswInfo { nullptr, vsw2.addVar, vsw2.addValue } });
		}
	}
	
	return;
}

KnowbugConfig::~KnowbugConfig()
{
	for ( auto const& info : vswInfo ) {
		if ( info.second.hDll ) FreeLibrary(info.second.hDll);
	}
	return;
}
