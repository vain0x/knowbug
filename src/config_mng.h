// 設定の読み込みと管理

#pragma once

#include "main.h"
#include <Windows.h>

#include <string>
#include <vector>
#include "module/mod_cstring.h"

struct KnowbugConfig
{
public:
	class Mng {
	public:
		void initialize() { KnowbugConfig::instance_.swap( std::unique_ptr<KnowbugConfig>(new KnowbugConfig()) ); }
		KnowbugConfig* operator ->() { return KnowbugConfig::instance_.get(); }
	};
public:
	string commonPath;
	
	// ini ファイルからロードされるもの
	std::vector<COLORREF> clrTypeText;
	bool bCustomDraw;
	int  maxlenVarinfo;
	int  tabwidth;
	bool bTopMost;
	
	string logPath;
	int  logMaxlen;
	bool bWordwrap;
	bool bResultNode;
	std::vector<string> extraTypeFormat;
	
	// スクリプトから設定されるもの
	
private:
	friend class Mng;
	static std::unique_ptr<KnowbugConfig> instance_;
	KnowbugConfig();
};

extern KnowbugConfig::Mng g_config;
