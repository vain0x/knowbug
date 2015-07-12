// macro for cmdlist

#ifndef IG_HPIMOD_CMDLIST_MACRO_H
#define IG_HPIMOD_CMDLIST_MACRO_H

// cmdlist 用のマクロ
// スクリプト側に提供するプラグインコマンドの一覧を、以下のマクロを使って記述しておき、C++ 側と HSP 側で共有する。
// 読み込む際のマクロの定義により、その一覧から、各コマンドに対するコードを生成させることができる。

// S, F, V はそれぞれ、そのコマンドが命令形式、関数形式、システム変数形式で使用できることを意味する。
// 命令形式と関数形式の両方で使用されるコマンドは、引数に ppResult を受け取るが、
// 命令形式で呼ばれたなら ppResult = nullptr となり、返値が無視される。
// 関数形式とシステム変数形式の両方で使用されるコマンドは、追加の引数 bSysvar を受け取り、これが真のときはシステム変数形式である。

// S, F, V のいずれでも使えないコマンドとは、byref のような単なるキーワードである。
// 使用例は cmd_call.(h|cpp) を参照のこと。

#ifdef _CmdlistModeProcess
# define HpiCmdlistBegin switch ( cmd ) {
# define HpiCmdlistEnd default: puterror(HSPERR_UNSUPPORTED_FUNCTION); }
# define HpiCmdlistSectionBegin(_Name) { using namespace _Name; //switch ( cmd ) {
# define HpiCmdlistSectionEnd } //}  switch
# define HpiCmd___(_Id, _Keyword) case Id::_Keyword: puterror(HSPERR_UNSUPPORTED_FUNCTION);
# if _CmdlistModeProcess == 'S'
#  define HpiCmdS__(_Id, _Keyword) case Id::_Keyword: _Keyword(); break;
#  define HpiCmdSF_ HpiCmdS__
#  define HpiCmdS_V HpiCmdS__
#  define HpiCmdSFV HpiCmdS__
#  define HpiCmd_F_ HpiCmd___
#  define HpiCmd__V HpiCmd___
#  define HpiCmd_FV HpiCmd___
# endif
# if _CmdlistModeProcess == 'F'
#  define HpiCmd_F_(_Id, _Keyword) case Id::_Keyword: return _Keyword(ppResult);
#  define HpiCmdSF_ HpiCmd_F_
#  define HpiCmd_FV HpiCmd_F_
#  define HpiCmdSFV HpiCmd_F_
#  define HpiCmdS__ HpiCmd___
#  define HpiCmd__V HpiCmd___
#  define HpiCmdS_V HpiCmd___
# endif
# if _CmdlistModeProcess == 'V'
#  define HpiCmd__V(_Id, _Keyword) case Id::_Keyword: return _Keyword(ppResult);
#  define HpiCmdS_V HpiCmd__V
#  define HpiCmd_FV(_Id, _Keyword) case Id::_Keyword: return _Keyword(ppResult, true);
#  define HpiCmdSFV HpiCmd_FV
#  define HpiCmdS__ HpiCmd___
#  define HpiCmd_F_ HpiCmd___
#  define HpiCmdSF_ HpiCmd___
# endif
# if !(_CmdlistModeProcess == 'S' || _CmdlistModeProcess == 'F' || _CmdlistModeProcess == 'V')
#  error "_CmdlistModeProcess must be 'S', 'F', or 'V'."
# endif
#else
# define HpiCmdlistBegin //
# define HpiCmdlistEnd //
# define HpiCmdlistSectionBegin(_Name) namespace _Name {
# define HpiCmdlistSectionEnd } // namespace
# define HpiCmdlistDefineId_(_Id, _Keyword) namespace Id { static int const _Keyword = _Id; }
# define HpiCmd___(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword);
# define HpiCmdS__(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern void _Keyword(); 
# define HpiCmd_F_(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult);
# define HpiCmdSF_(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult = nullptr);
# define HpiCmd__V(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult = nullptr);
# define HpiCmdS_V(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult = nullptr);
# define HpiCmd_FV(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult = nullptr, bool bSysvar = false);
# define HpiCmdSFV(_Id, _Keyword) HpiCmdlistDefineId_(_Id, _Keyword); extern int _Keyword(PDAT** ppResult = nullptr, bool bSysvar = false);
#endif

#else

// remove all macroes when second load

#undef HpiCmdlistBegin
#undef HpiCmdlistEnd
#undef HpiCmdlistSectionBegin
#undef HpiCmdlistSectionEnd
#undef HpiCmdlistDefineId_
#undef HpiCmd___
#undef HpiCmdS__
#undef HpiCmd_F_
#undef HpiCmdSF_
#undef HpiCmd__V
#undef HpiCmdS_V
#undef HpiCmd_FV
#undef HpiCmdSFV

#undef IG_HPIMOD_CMDLIST_MACRO_H
#endif
