
#include "config_mng.h"
#include "module/CIni.h"

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
	CIni ini( ownpath_full.c_str() );
	
	// common フォルダへのパス
	commonPath = strf( "%scommon", ownpath );
	
	// カスタムドローするかどうか
	bCustomDraw = ini.getBool( "ColorType", "bCustomDraw", false );
	
	char stmp[256] = "";
	
	// 型ごとの色
	clrTypeText.reserve( HSPVAR_FLAG_USERDEF );
	
	for ( int i = 0; i < HSPVAR_FLAG_USERDEF; ++ i ) {
		sprintf_s( stmp, "text#%d", i );
		clrTypeText.push_back(
			ini.getInt( "ColorType", stmp, RGB(0, 0, 0) )
		);
	}
	
	// 最大表示データ量
	maxlenVarinfo = ini.getInt( "Varinfo", "maxlen", 0x1000 - 1 );
	logMaxlen     = ini.getInt( "Log",     "maxlen", 0x20000 );
	
	// タブ文字幅
	tabwidth  = ini.getInt( "Interface", "tabwidth", 4 );
	
	// 右端で折り返すか否か
	bWordwrap = ini.getBool( "Interface", "bWordwrap", false );
	
	// 最前面ウィンドウか否か
	bTopMost = ini.getBool( "Window", "bTopMost", false );
	
	// 自動保存パス
	logPath = ini.getString( "Log", "autoSavePath", "" );
	
	// 返値ノードを使うか
	bResultNode = ini.getBool( "Varinfo", "useResultNode", false );
	
	return;
}
