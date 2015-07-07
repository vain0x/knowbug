// knowbug 公開API

#ifndef IG_KNOWBUG_AS
#define IG_KNOWBUG_AS

#ifdef _DEBUG
#module

#ifdef __hsp64__
 #uselib "hsp3debug_64.dll"
 #cfunc _knowbug_hwnd@__knowbug "knowbug_hwnd"
 #func knowbug_writeVarinfoString "knowbug_writeVarinfoString" sptr, pval, pval
 #func knowbug_getCurrentModcmdName "knowbug_getCurrentModcmdName" sptr, int, prefstr
#else //defined(__hsp64__)
 #uselib "hsp3debug.dll"
 #cfunc _knowbug_hwnd@__knowbug "_knowbug_hwnd@0"
 #func knowbug_writeVarinfoString "_knowbug_writeVarinfoString@12" sptr, pval, pval
 #func knowbug_getCurrentModcmdName "_knowbug_getCurrentModcmdName@12" sptr, int, prefstr
#endif //defined(__hsp64__)

//------------------------------------------------
// knowbug_hwnd
// knowbug のウィンドウハンドル
//------------------------------------------------
#define global knowbug_hwnd _knowbug_hwnd@__knowbug()

//------------------------------------------------
// knowbug_varinfstr( 変数 )
// 変数の詳細情報を表す文字列を返す
//------------------------------------------------
#define global ctype knowbug_varinfstr(%1) varinfstr@__knowbug(%1, "%1")
#defcfunc varinfstr@__knowbug array v, str _name,  local name, local buf
	name = _name
	sdim buf, 65535
	knowbug_writeVarinfoString strtrim(name), v, buf
	return buf
	
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

#else // defined(_DEBUG)

#define global knowbug_hwnd 0
#define global ctype knowbug_varinfstr(%1) ""
#define global __func__ ""

#endif // defined(_DEBUG)

#endif
