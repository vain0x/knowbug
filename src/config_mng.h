// 設定の読み込みと管理

#pragma once

#include "encoding.h"
#include "main.h"
#include "module/Singleton.h"
#include "module/handle_deleter.hpp"

struct KnowbugConfig : public Singleton<KnowbugConfig>
{
	friend class Singleton<KnowbugConfig>;

public:
	//properties from ini (see it for detail)

	OsString hspDir;
	bool bTopMost;
	bool viewPosXIsDefault, viewPosYIsDefault;
	int viewPosX, viewPosY;
	int viewSizeX, viewSizeY;
	int  tabwidth;
	OsString fontFamily;
	int fontSize;
	bool fontAntialias;
	OsString logPath;
	bool warnsBeforeClearingLog;
	bool scrollsLogAutomatically;
#ifdef with_WrapCall
	bool logsInvocation;
#endif

	auto commonPath() const -> OsString {
		return OsString{ hspDir + TEXT("common") };
	}

	auto selfPath() const -> OsString {
		return OsString{ hspDir + TEXT("knowbug.ini") };
	}

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
