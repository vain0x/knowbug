// 設定の読み込みと管理

#pragma once

#include "main.h"
#include <Windows.h>

#include <string>
#include <array>
#include <map>
#include <memory>
#include "module/strf.h"

#include <functional>
#include "ExVardataString.h"

namespace Detail
{
	struct moduleDeleter { using pointer = HMODULE; void operator()(HMODULE p) { FreeLibrary(p); } };
};

struct KnowbugConfig
{
public:
	class Mng {
	public:
		void initialize() { KnowbugConfig::instance_.reset(new KnowbugConfig()); }
		KnowbugConfig* operator ->() { return KnowbugConfig::instance_.get(); }
	};

	using moduleHandle_t = std::unique_ptr<HMODULE, Detail::moduleDeleter>;
	using VswInfo = std::tuple<moduleHandle_t, addVarUserdef_t, addValueUserdef_t>;

public:
	string commonPath;
	
	// ini ファイルからロードされるもの
	int  maxlenVarinfo;
	int  tabwidth;
	bool bTopMost;
	
	string logPath;
	int  logMaxlen;
	//bool bWordwrap;
	bool bResultNode;

	bool bCustomDraw;
	std::array<COLORREF, HSPVAR_FLAG_USERDEF> clrText;
	std::map<string, COLORREF> clrTextExtra;

	std::map<string, VswInfo> vswInfo;

	// スクリプトから設定されるもの
	
private:
	friend class Mng;
	static std::unique_ptr<KnowbugConfig> instance_;
	KnowbugConfig();
};

extern KnowbugConfig::Mng g_config;

// 返値ノード機能を使うかどうか
static bool utilizeResultNodes() { return g_config->bResultNode; }
