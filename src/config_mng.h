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
	struct VswInfo
	{
		module_handle_t inst;
		addVarUserdef_t addVar;
		addValueUserdef_t addValue;

	public:
		//workaround for VC++2013
		VswInfo()
			: inst {}, addVar {}, addValue {}
		{}
		VswInfo(VswInfo&& r)
		{
			*this = std::move(r);
		}
		VswInfo(module_handle_t&& inst, addVarUserdef_t addVar, addValueUserdef_t addValue)
			: inst(std::move(inst)), addVar(addVar), addValue(addValue)
		{}
		auto operator=(VswInfo&& r) -> VswInfo&
		{
			inst = std::move(r.inst); addVar = r.addVar; addValue = r.addValue;
			return *this;
		}
	};

public:
	//properties from ini (see it for detail)

	string hspDir;
	bool bTopMost;
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
	std::vector<VswInfo> vswInfo;
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
	bool tryRegisterVswInfo(string const& vtname, VswInfo vswi);

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
