﻿//ごった煮:
//windows APIまわりの宣言
//hspsdkまわりの宣言
//knowbugの宣言

#ifndef IG_HSP3DBGWIN_KNOWBUG_H
#define IG_HSP3DBGWIN_KNOWBUG_H

#include <Windows.h>
#undef min
#undef max

#include "hpiutil/hpiutil.hpp"
#include "DebugInfo.h"
#include "encoding.h"
#include "module/utility.h"
#include "StepController.h"

using hpiutil::vartype_t;
using hpiutil::varmode_t;
using hpiutil::label_t;
using hpiutil::csptr_t;
using hpiutil::stdat_t;
using hpiutil::stprm_t;
using hpiutil::HSPVAR_FLAG_COMOBJ;
using hpiutil::HSPVAR_FLAG_VARIANT;

//extern HSPCTX* ctx; // declared and defined in hsp3plugin.(h/cpp)
//extern HSPEXINFO* exinfo;

class HspRuntime;
class Logger;
class SourceFileResolver;

// FIXME: インターフェイスを抽出
// knowbug コントロール
namespace Knowbug
{
	extern auto getInstance() -> HINSTANCE;

	extern auto get_hsp_runtime() -> HspRuntime&;

	extern auto get_logger()->std::shared_ptr<Logger>;

	extern auto get_source_file_resolver()->std::shared_ptr<SourceFileResolver>;

	extern void step_run(StepControl step_control);
	extern bool continueConditionalRun();

	extern void logmes(char const* msg); //自動改行なし
	extern void logmes(OsStringView const& msg);
	extern void logmesWarning(char const* msg);
	extern void logmesWarning(OsStringView const& msg);

	extern void open_current_script_file();
	extern void open_config_file();
	extern void open_knowbug_repository();

} // namespace Knowbug

#endif
