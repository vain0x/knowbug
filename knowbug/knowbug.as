// knowbug 公開API

#ifndef IG_KNOWBUG_AS
#define IG_KNOWBUG_AS

#ifdef _DEBUG

#uselib "hsp3debug.dll"
#cfunc _knowbug_hwnd@__knowbug "_knowbug_hwnd@0"

//------------------------------------------------
// knowbug_hwnd
// knowbug のウィンドウハンドル
//------------------------------------------------
#define global knowbug_hwnd _knowbug_hwnd@__knowbug()

#module

#func knowbug_getVarinfoString "_knowbug_getVarinfoString@12" sptr, pval,  prefstr
#func knowbug_getCurrentModcmdName "_knowbug_getCurrentModcmdName@12" sptr, int, prefstr

//------------------------------------------------
// knowbug_varinfstr( 変数 )
// 変数の詳細情報を表す文字列を返す
//------------------------------------------------
#define global ctype knowbug_varinfstr(%1) varinfstr@__knowbug(%1, "%1")
#defcfunc varinfstr@__knowbug array v, str _name,  local name
	name = _name
	knowbug_getVarinfoString strtrim(name), v
	return refstr
	
//------------------------------------------------
// __func__
// 今呼び出されているユーザ定義命令・関数の名前
// 何も呼び出されていない場合、または WrapCall が機能していない場合は "main" になる。
//------------------------------------------------
#ifndef __func__
#define global __func__ (_lastModcmdName@__knowbug())
#defcfunc _lastModcmdName@__knowbug
	knowbug_getCurrentModcmdName "main", 1
	return refstr
#endif

#global

#module
//サンプル exNN_custom_node.hsp を参照
#func addCustomNode "_knowbug_addCustomNode@12" sptr, sptr, var

#define global knowbug_addCustomNode(%1,%2,%3) lbVsw@__knowbug = (%3) : addCustomNodeImpl@__knowbug %1, %2, lbVsw@__knowbug
#deffunc addCustomNodeImpl@__knowbug str name, str parent, var lb
	addCustomNode name, parent, lb
	return

#cfunc global knowbug_getCurrentVswriter "_knowbug_getCurrentVswriter@0"
#func global knowbugVsw_beforeReturn "_knowbugVsw_beforeReturn@0"
#func global knowbugVsw_catLeaf       "_knowbugVsw_catLeaf@12"      int, str, str
#func global knowbugVsw_catLeafExtra  "_knowbugVsw_catLeafExtra@12" int, str, str
#func global knowbugVsw_catAttribute  "_knowbugVsw_catAttribute@12" int, str, str
#func global knowbugVsw_catNodeBegin  "_knowbugVsw_catNodeBegin@12" int, str, str
#func global knowbugVsw_catNodeEnd    "_knowbugVsw_catNodeEnd@8"    int, str
#func global knowbugVsw_addVar        "_knowbugVsw_addVar@12"       int, str, pval
#func global knowbugVsw_addVarScalar  "_knowbugVsw_addVarScalar@16" int, str, pval, int
#func global knowbugVsw_addVarArray   "_knowbugVsw_addVarArray@12"  int, str, pval
#func global knowbugVsw_addValue      "_knowbugVsw_addValue@12"     int, str, int, int
#func global knowbugVsw_addSysvar     "_knowbugVsw_addSysvar@8"     int, str

#define global knowbugVsw_return(%1) knowbugVsw_beforeReturn : assert : return

#global

#else	// defined(_DEBUG)

#define global knowbug_hwnd 0
#define global ctype knowbug_varinfstr(%1) ""
#define global __func__ ""

#endif	// defined(_DEBUG)

#endif
