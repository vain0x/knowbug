//ごった煮:
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

class HspObjectPath;
class HspRuntime;
class KnowbugApp;
class KnowbugView;
class SourceFileResolver;

class KnowbugApp {
public:
	static auto instance() -> std::shared_ptr<KnowbugApp>;

	virtual ~KnowbugApp() {
	}

	virtual auto view() -> KnowbugView& = 0;

	virtual void step_run(StepControl step_control) = 0;

	virtual void add_object_text_to_log(HspObjectPath const& path) = 0;

	virtual void clear_log() = 0;

	virtual void save_log() = 0;

	virtual void open_current_script_file() = 0;

	virtual void open_config_file() = 0;

	virtual void open_knowbug_repository() = 0;
};

#endif
