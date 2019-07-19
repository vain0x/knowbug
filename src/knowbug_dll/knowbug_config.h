// 設定の読み込みと管理

#pragma once

#include <memory>
#include "../knowbug_core/encoding.h"
#include "../knowbug_core/platform.h"

struct KnowbugConfig;

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

	auto commonPath() const -> OsString {
		return OsString{ hspDir + TEXT("common") };
	}

	auto selfPath() const->OsString;

	static auto create()->std::unique_ptr<KnowbugConfig>;
};
