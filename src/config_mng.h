// 設定の読み込みと管理

#pragma once

#include "main.h"
#include "module/Singleton.h"
#include "module/handle_deleter.hpp"
#include "ExVardataString.h"

struct KnowbugConfig : public Singleton<KnowbugConfig>
{
	friend class Singleton<KnowbugConfig>;

public:
	//properties from ini (see it for detail)

	string hspDir;
	bool bTopMost;
	bool viewPosXIsDefault, viewPosYIsDefault;
	int viewPosX, viewPosY;
	int viewSizeX, viewSizeY;
	int  tabwidth;
	string fontFamily;
	int fontSize;
	bool fontAntialias;
	int  maxLength, infiniteNest;
	bool showsVariableAddress, showsVariableSize, showsVariableDump;
	string prefixHiddenModule;
	bool cachesVardataString;
	bool bResultNode;
	bool bCustomDraw;
	std::array<COLORREF, HSPVAR_FLAG_USERDEF> clrText;
	unordered_map<string, COLORREF> clrTextExtra;
	string logPath;
	bool warnsBeforeClearingLog;
	bool scrollsLogAutomatically;
#ifdef with_WrapCall
	bool logsInvocation;
#endif

	auto commonPath() const -> string { return hspDir + "common"; }
	auto selfPath() const -> string { return hspDir + "knowbug.ini"; }

private:
	KnowbugConfig();

public:
	//to jusitify existent codes (such as g_config->property)
	class SingletonAccessor {
	public:
		void initialize() { instance(); }
		auto operator->() -> KnowbugConfig* { return &instance(); }
	};
};

extern KnowbugConfig::SingletonAccessor g_config;

static bool usesResultNodes() { return g_config->bResultNode; }
