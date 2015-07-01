// knowbug 公開API

#ifndef IG_KNOWBUG_AS
#define IG_KNOWBUG_AS

#ifdef _DEBUG

#uselib "hsp3debug.dll"
#cfunc _knowbug_hwnd@__knowbug "_knowbug_hwnd@0"
// knowbug_hwnd
// knowbug のウィンドウハンドル
#define global knowbug_hwnd _knowbug_hwnd@__knowbug()

#module

#func knowbug_getVarinfoString "_knowbug_getVarinfoString@12" sptr, pval,  prefstr
#func knowbug_getCurrentModcmdName "_knowbug_getCurrentModcmdName@12" sptr, int, prefstr

// varinfstr( 変数 )
// 変数の詳細情報を表す文字列を返す
#define global ctype varinfstr(%1) varinfstr@__knowbug(%1, "%1")
#defcfunc varinfstr@__knowbug array v, str _name,  local name
	name = _name
	knowbug_getVarinfoString strtrim(name), v
	return refstr
	
// __func__
// 今呼び出されているユーザ定義命令・関数の名前
// 何も呼び出されていない場合、または WrapCall が機能していない場合は "main" になる。
#define global __func__ (_lastModcmdName@__knowbug())
#defcfunc _lastModcmdName@__knowbug
	knowbug_getCurrentModcmdName "main", 1
	return refstr
	
#global

#else	// defined(_DEBUG)

#define global knowbug_hwnd 0
#define global ctype varinfstr(%1) ""
#define global __func__ ""

#endif	// defined(_DEBUG)

#endif
