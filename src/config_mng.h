// 設定の読み込みと管理

#pragma once

#include "encoding.h"
#include "main.h"
#include "module/Singleton.h"
#include "module/handle_deleter.hpp"

struct KnowbugConfig;

extern std::unique_ptr<KnowbugConfig> g_knowbug_config;

// knowbug のすべての設定。package/knowbug.ini を参照。
struct KnowbugConfig {
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

	auto selfPath() const->OsString;

	static void initialize();
	static auto load(OsString&& hsp_dir)->KnowbugConfig;

	// `g_config->member` のような記述のコンパイルを通すためのもの。
	struct SingletonAccessor {
		auto operator->() -> KnowbugConfig* {
			assert(g_knowbug_config != nullptr);
			return g_knowbug_config.get();
		}
	};
};

extern KnowbugConfig::SingletonAccessor g_config;
