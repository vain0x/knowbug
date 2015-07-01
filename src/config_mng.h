// 設定の読み込みと管理

#pragma once

#include "main.h"
#include <Windows.h>

#include <string>
#include <vector>
#include <memory>
#include "module/strf.h"

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

// 返値ノード機能を使うかどうか
static bool utilizeResultNodes() { return g_config->bResultNode; }
