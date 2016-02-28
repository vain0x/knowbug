// 設定の読み込みと管理

#pragma once

#include "main.h"
#include "module/Singleton.h"
#include "module/handle_deleter.hpp"
#include "cpp-feather-ini-parser/INI.h"
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
	// See knowbug_default.ini for details

#define KNOWBUG_CONFIG_FIELD(_Iden, _Type)   \
		auto _Iden() const -> _Type          \
		{ return load<_Type>("", #_Iden); }  \
		auto _Iden(_Type const& val) -> void \
		{ return save<_Type>("", #_Iden, val); } //
	
	KNOWBUG_CONFIG_FIELD(tabWidth                , bool  );
	KNOWBUG_CONFIG_FIELD(fontFamily              , string);
	KNOWBUG_CONFIG_FIELD(fontSize                , int   );
	KNOWBUG_CONFIG_FIELD(fontAntialias           , bool  );
	KNOWBUG_CONFIG_FIELD(windowTopmost           , bool  );
	KNOWBUG_CONFIG_FIELD(viewWindowSizeX         , int   );
	KNOWBUG_CONFIG_FIELD(viewWindowSizeY         , int   );
	KNOWBUG_CONFIG_FIELD(varinfoMaxLen           , int   );
	KNOWBUG_CONFIG_FIELD(varinfoMaxNest          , int   );
	KNOWBUG_CONFIG_FIELD(usesResultNodes         , bool  );
	KNOWBUG_CONFIG_FIELD(showsVarAddr            , bool  );
	KNOWBUG_CONFIG_FIELD(showsVarSize            , bool  );
	KNOWBUG_CONFIG_FIELD(showsVarDump            , bool  );
	KNOWBUG_CONFIG_FIELD(prefixHiddenModule      , string);
	KNOWBUG_CONFIG_FIELD(logAutoSavePath         , string);
	KNOWBUG_CONFIG_FIELD(logMaxLen               , int   );
	KNOWBUG_CONFIG_FIELD(warnsBeforeClearingLog  , bool  );
	KNOWBUG_CONFIG_FIELD(scrollsLogAutomatically , bool  );
	KNOWBUG_CONFIG_FIELD(enableCustomDraw        , bool  );

#ifdef with_WrapCall
	KNOWBUG_CONFIG_FIELD(logsInvocation          , bool  );
#endif

#undef KNOWBUG_CONFIG_FIELD

	auto commonPath() const -> string { return hspDir + "common"; }
	auto selfPath() const -> string { return hspDir + "knowbug.ini"; }

private:
	KnowbugConfig();
	bool tryRegisterVswInfo(string const& vtname, VswInfo vswi);

	template<typename T>
	struct typeTag {};

	template<typename T>
	auto load(char const* sec, char const* key) const -> T
	{
		return ini_.get(sec, key, T {});
	}

	template<typename T>
	void save(char const* sec, char const* key, T const& val)
	{
		ini_.select(sec);
		ini_.set(key, val);
	}

private:
	mutable INI<> ini_;

	string hspDir;

public:
	std::array<COLORREF, HSPVAR_FLAG_USERDEF> clrText;
	unordered_map<string, COLORREF> clrTextExtra;
	std::vector<VswInfo> vswInfo;

public:
	//to jusitify existent codes (such as g_config->property)
	class SingletonAccessor {
	public:
		void initialize() { instance(); }
		auto operator->() -> KnowbugConfig* { return &instance(); }
	};
};

extern KnowbugConfig::SingletonAccessor g_config;

static bool usesResultNodes() { return g_config->usesResultNodes(); }
