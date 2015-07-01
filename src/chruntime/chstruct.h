// clhsp - declaration of struct

#ifndef IG_CLHSP_STRUCT_H
#define IG_CLHSP_STRUCT_H

#include "module/defdef.h"
#include "module/mod_debug.h"

#include "chvar_core.h"
#include "chdebug.h"

// command type
#define TYPE_MARK     0
#define TYPE_VAR      1
#define TYPE_STRING   2
#define TYPE_DNUM     3
#define TYPE_INUM     4
#define TYPE_STRUCT   5
#define TYPE_XLABEL   6
#define TYPE_LABEL    7
#define TYPE_INTCMD   8
#define TYPE_EXTCMD   9
#define TYPE_EXTSYSVAR 10
#define TYPE_CMPCMD  11
#define TYPE_MODCMD  12
#define TYPE_INTFUNC 13
#define TYPE_SYSVAR  14
#define TYPE_PROGCMD 15
#define TYPE_DLLFUNC 16
#define TYPE_DLLCTRL 17
#define TYPE_VARTYPE 18			// 型識別子
#define TYPE_DECLARE 19			// 宣言
#define TYPE_LITID   20			// リティッド (実行時には存在しない)
#define TYPE_ALIAS   21			// エイリアス (〃)
#define TYPE_USERDEF 22
#define TYPE_MAX     TYPE_USERDEF

#define TYPE_ERROR     (-1)
#define TYPE_CALCERROR (-2)

#define PARAM_OK       0
#define PARAM_SPLIT    (-1)
#define PARAM_END      (-2)
#define PARAM_DEFAULT  (-3)
#define PARAM_ENDSPLIT (-4)

#define HSP3_FUNC_MAX  TYPE_USERDEF
#define HSP3_TYPE_USER TYPE_USERDEF

#define EXFLG_1 0x2000
#define EXFLG_2 0x4000
#define CSTYPE  0x1fff

struct CodeValue
{
	int type;
	int val;
};

struct HSPHED
{
	// clhsp header structure
	
	char	h1;					// magic code1
	char	h2;					// magic code2
	char	h3;					// magic code3
	char	h4;					// magic code4
	int		version;			// version number info
	int		max_val;			// max count of VAL Object
	int		allsize;			// total file size

	int		pt_cs;				// ptr to Code Segment
	int		max_cs;				// size of CS
	int		pt_ds;				// ptr to Data Segment
	int		max_ds;				// size of DS

	int		pt_ot;				// ptr to Object Temp
	int		max_ot;				// size of OT
	int		pt_dinfo;			// ptr to Debug Info
	int		max_dinfo;			// size of DI

	int		pt_linfo;			// ptr to LibInfo(2.3)
	int		max_linfo;			// size of LibInfo(2.3)
	int		pt_finfo;			// ptr to FuncInfo(2.3)
	int		max_finfo;			// size of FuncInfo(2.3)

	int		pt_minfo;			// ptr to ModInfo(2.5)
	int		max_minfo;			// size of ModInfo(2.5)
	int		pt_finfo2;			// ptr to FuncInfo2(2.5)
	int		max_finfo2;			// size of FuncInfo2(2.5)
	
	int		pt_hpidat;			// ptr to HPIDAT(3.0)
	short	max_hpi;			// size of HPIDAT(3.0)
	short	max_varhpi;			// Num of Vartype Plugins(3.0)
	int		bootoption;			// bootup options
	int		runtime;			// ptr to runtime name

};

//#define HSPHED_BOOTOPT_WINHIDE 2			// 起動時ウインドゥ非表示
//#define HSPHED_BOOTOPT_DIRSAVE 4			// 起動時カレントディレクトリ変更なし
#define HSPHED_BOOTOPT_DEBUGWIN 1			// 起動時デバッグウインドウ表示
//#define HSPHED_BOOTOPT_SAVER 0x100		// スクリーンセーバー
#define HSPHED_BOOTOPT_RUNTIME 0x1000		// 動的ランタイムを有効にする
#define HSPHED_BOOTOPT_DYNALOG 0x2000		// + 動作記録を行う

#define HPIDAT_FLAG_TYPEFUNC 0
#define HPIDAT_FLAG_VARFUNC 1
#define HPIDAT_FLAG_DLLFUNC 2

struct HPIDAT
{
	short flag;				// flag info
	short option;
	int   libname;			// lib name index (DS)
	int   funcname;			// function name index (DS)
	void* libptr;			// lib handle
};


#define LIBDAT_FLAG_NONE 0
#define LIBDAT_FLAG_DLL 1
#define LIBDAT_FLAG_DLLINIT 2
#define LIBDAT_FLAG_MODULE 3
#define LIBDAT_FLAG_COMOBJ 4

struct LIBDAT
{
	int   flag;				// initalize flag
	int   nameidx;			// function name index (DS)
							// Interface IID ( Com Object )
	void* hlib;				// Lib handle
	int   clsid;			// CLSID (DS) ( Com Object )
};

// multi parameter type
namespace PrmType
{
	static const short
		None       = 0,			// --
		Label      = 1,			// label
		Str        = 2,			// str
		Double     = 3,			// double
		Int        = 4,			// int
		ModInst    = 5,			// modinst
		LValue     = 8,			// (lvalue)
		Vector     = 9,			// vector
		Array      = 10,		// array
		Block      = 11,		// block
		Void       = 12,		// void
//		Function   = 13,		// function
		ModclsBias = 15,		// (mod-cls)
		
		LocalVar   = (-1),		// local
		ByRef      = LValue,	// byref
		Var        = LValue,	// var
		Float      = (-4),		// float
		Modcls     = (-5),		// (modcls)
		LocalStr   = (-6),		// (str)
		Thismod    = (-7),		// modvar (thismod)
		pPVal      = (-8),		// pval
		pBMSCR     = (-9),		// bmscr
		VarPtr     = (-10),		// var (#dllfunc)
		ModInit    = (-11),		// modinit
		
		IObjectVar = (-12),		// comobj
		LocalWStr  = (-13),		// 
		FlexSPtr   = (-14),		// 
		FlexWPtr   = (-15),		// 
		pRefStr    = (-16),		// prefstr
		pExinfo    = (-17),		// pexinfo
		pDpmInfo   = (-18),		// pdpminfo
		NullPtr    = (-19),		// nullptr
		ModTerm    = (-20),		// modterm

		Any        = (-21),		// any
		Flex       = (-22),		// flex (...)
		Bool       = (-23),		// bool

//		phWnd      = (-14),
//		phDc       = (-15),
//		phInstance = (-16),
		_;
}

#define STRUCTPRM_SUBID_ARG        (-1)		// [PRM] #deffunc の仮引数 (explicit)
#define STRUCTPRM_SUBID_STID       (-2)		// [PRM] (funcと関係がありそうだが、不明)
#define STRUCTPRM_SUBID_DLL        (-3)		// [DAT] Dll関数
#define STRUCTPRM_SUBID_DLLINIT    (-4)		// [DAT] ↑の実行時初期化が完了済みである状態
#define STRUCTPRM_SUBID_OLDDLL     (-5)		// [DAT] Dll関数 (「#func ident ".." 数値」という古い形式)
#define STRUCTPRM_SUBID_OLDDLLINIT (-6)		// [DAT] ↑の実行時初期化が完了済みである状態
#define STRUCTPRM_SUBID_COMOBJ     (-7)		// [DAT] #comfunc など

#define TYPE_OFFSET_COMOBJ 0x1000

struct STRUCTPRM
{
	short mptype;				// parameter type
	short subid;				// id_finfo
	int   nameidx;				// name index (DS)
	union {
		int idxOwner;			// オーナーである STRUCTDAT の finfo-idx
		int offset;				// offset from top
		int idxfinfo_init;		// init 関数の STRUCTDAT の finfo-idx
	};
#ifdef USE_DEFAULT_ARG
	int dsidx_defval;			// 省略時引数への ds-idx
#endif
};

//	DLL function flags
#define STRUCTDAT_OT_NONE      0x0000
#define STRUCTDAT_OT_CLEANUP   0x0001
#define STRUCTDAT_OT_STATEMENT 0x0002
#define STRUCTDAT_OT_FUNCTION  0x0004
#define STRUCTDAT_OT_ANYWAY ( STRUCTDAT_OT_STATEMENT | STRUCTDAT_OT_FUNCTION )	// + ( 命令・関数形式問わない )

//	Module function flags
#define STRUCTDAT_INDEX_CMD    (-1)		// ユーザ定義コマンド
#define STRUCTDAT_INDEX_MODCLS (-2)		// module-class

#define STRUCTDAT_FUNCFLAG_CLEANUP STRUCTDAT_FUNCFLAG_TERMER
#define STRUCTDAT_FUNCFLAG_TERMER 0x10000	// 後処理命令
#define STRUCTDAT_FUNCFLAG_INITER 0x20000	// 前処理命令

// function,module specific data
struct STRUCTDAT
{
	short	index;				// base LIBDAT index
	short	subid;				// struct index
	int		prmindex;			// STRUCTPRM index(MINFO)
	int		prmmax;				// number of STRUCTPRM
	int		nameidx;			// name index (DS)
	int		size;				// struct size (stack)
	union {
		int otindex;			// OT index(Module) / cleanup flag(Dll)
		int idxfinfo_term;		// term 関数の STRUCTDAT の finfo-idx
	};
	union {
		void* proc;				// proc address
		int   funcflag;			// function flags(Module)
	};
	PrmStk* prmstk;				// 現在呼び出されているこの関数の内、最後に呼び出されているもの
};

namespace IrqFlag
{
	static const int
		None    = 0,
		Disable = 1,
		Enable  = 2;
}

namespace IrqOpt
{
	static const int
		Goto     = 0,
		Gosub    = 1,
		CallBack = 2;
}

// Stack info for DLL Parameter
struct MPStack
{
	char*  prmbuf;
	char** prmstk;
	int curstk;
	void* vptr;
};


struct IrqData
{
	typedef void (*pfCallback_t)(IrqData*, int, int);
	
	short   flag;				// flag
	short   opt;				// option value
	int     custom;				// custom message value
	int     custom2;			// custom message value2
	int     iparam;				// iparam option
	csptr_t lbDest;				// jump ptr
	pfCallback_t callback;		// IRQ callback function
};

struct HSPCTX;

//	Plugin info data (3.1 or later)
struct HSPEXINFO
{
	// HSP internal info data
	short ver;			// Version Code
	short min;			// Minor Version
	
	int* _padding;		// Not Use
	char* pstr;			// String Buffer (master)
	char* stmp;			// String Buffer (sub)
	ScrValue** mpval;	// Temporary Value
	
	int* actscr;		// Active Window ID
	int* nptype;		// Next Parameter Type
	int* npval;			// Next Parameter Value
	int* strsize;		// StrSize Buffer
	char* refstr;		// RefStr Buffer
	
	void* (*HspFunc_prm_getv)(void);
	int (*HspFunc_prm_geti)(void);
	int (*HspFunc_prm_getdi)( const int defval );
	char* (*HspFunc_prm_gets)(void);
	char* (*HspFunc_prm_getds)( const char* defstr );
//	int (*HspFunc_val_realloc)( PVal *pv, int size, int mode );
	int (*HspFunc_fread)( char* fname, void* readmem, int rlen, int seekofs );
	int (*HspFunc_fsize)( char* fname );
	void* (*HspFunc_getbmscr)( int wid );
	int (*HspFunc_getobj)( int wid, int id, void* inf );
	int (*HspFunc_setobj)( int wid, int id, const void* inf );
	
	int* npexflg;	// Next Parameter ExFlg
	HSPCTX* ctx;	// HSP context ptr
	
	// Enhanced data
	int (*HspFunc_addobj)( int wid );
	void (*HspFunc_puterror)( RunError error );
	VarProc* (*HspFunc_getproc)( vartype_t vt );
	VarProc* (*HspFunc_seekproc)( const char* name );
	/*
	void (*HspFunc_prm_next)(void);
	int (*HspFunc_prm_get)(void);
	double (*HspFunc_prm_getd)(void);
	double (*HspFunc_prm_getdd)( double defval );
	csptr_t (*HspFunc_prm_getlb)(void);
	PVal *(*HspFunc_prm_getpval)(void);
	APTR (*HspFunc_prm_getva)( PVal** pval );
	void (*HspFunc_prm_setva)( PVal *pval, APTR aptr, int type, const void* ptr );
	//*/
	char* (*HspFunc_malloc)( size_t size );
	void (*HspFunc_free)( void* ptr );
	char* (*HspFunc_expand)( void* ptr, size_t size );
	IrqData *(*HspFunc_addirq)(void);
	int (*HspFunc_hspevent)( int event, int prm1, int prm2, void* prm3 );
	void (*HspFunc_registvar)( vartype_t vt, pfVarProcInit_t func );
	void (*HspFunc_setpc)( const csptr_t pc );
	void (*HspFunc_call)( const csptr_t pc );
	void (*HspFunc_mref)( ScrValue* pVal, int prm );
	
	/*
	void (*HspFunc_dim)( PVal *pval, int flag, int len0, int len1, int len2, int len3, int len4 );
	void (*HspFunc_redim)( PVal *pval, int lenid, int len );
	void (*HspFunc_array)( PVal *pval, int offset );
	//*/
	
	char* (*HspFunc_varname)( int id );
	int (*HspFunc_seekvar)( const char* name );
	
//	ModInst   *(*HspFunc_new_modinst)( STRUCTDAT* pStDat, void* members );
//	GcObjRef *(*HspFunc_add_object_ref)( Object* obj );
//	void (*HspFunc_remove_object_ref)( GcObjRef *ref );
	
};

#define HSP3_REPEAT_MAX 32
#define ETRLOOP     ((int)0x80000000)

struct LOOPDAT
{
	int time;		// loop times left
	int cnt;		// count
	int step;		// count add value
	csptr_t pt;		// loop start ptr
};

//typedef LOOPDAT LoopInfo;

enum LoopType
{
	LoopType_None = 0,
	LoopType_Repeat,
	LoopType_Foreach,
	LoopType_While,
	LoopType_Forever,
//	LoopType_For,
	LoopType_MAX
};

struct LoopInfo
{
	LoopType lptype;
	int cnt;
	label_t lbBegin;		// ループ内部の先頭
	label_t lbEnd;			// ループ終了地点 (loop 命令の位置)
	label_t lbElse;			// else 地点
	
	// 拡張データ
	union {
		int times;			// repeat
	} exdata;
};

// switch 文の実行時データ
#define MAX_SWITCH_NEST 32
struct SwitchInfo
{
	ushort flag;
	ushort _opt;			// (reserved)
	label_t lbBegin;		// switch の次の位置
	label_t lbNext;			// 次の case, swend の位置)
	label_t lbEnd;			// swend の位置
	
	ScrValue* pVal;			// 比較元の値
	
public:
	struct Flag {
		static const ushort
			None     = 0x0000,
			Finished = 0x0001,
			Falling  = 0x0002;
	};
};

// ロングジャンプデータ
struct ChLongJump
{
	label_t dest;		// ジャンプ先
	ushort sublev;		// dest の sublev ( 巻き戻すときに参照する )
	ushort looplev;		// dest の looplev
	ushort nestSwitch;	// dest の nestSwitch
};
#define HSP3_LONGJUMP_MAX 16

// 実行モード
enum RunMode_t
{
	RunMode_Run = 0,
	RunMode_Wait,
	RunMode_AWait,
	RunMode_Stop,
	RunMode_End,
	RunMode_Error,
	RunMode_Return,
	RunMode_IntJump,
	RunMode_Assert,
	RunMode_Logmes,
	RunMode_ExitRun,
	RunMode_Max
};

struct HSPCTX
{
	//	HSP Context
	//
	HSPHED* hsphed;						// HSP object file header
	csptr_t mcs;						// current code segment ptr
	csptr_t mem_mcs;					// code segment ptr
	char*   mem_mds;					// data segment ptr
	uchar*  mem_di;						// Debug info ptr
	int* mem_ot;						// object temp segment ptr
	
	IrqData* mem_irq;					// IRQ data ptr
	int irqmax;							// IRQ data count
	int iparam;							// IRQ Info data1
	int wparam;							// IRQ Info data2
	int lparam;							// IRQ Info data3
	
	ScrVar** mem_var;					// var storage index
	int runmode;						// HSP execute mode
	int waitcount;						// counter for wait
	int waitbase;						// wait sleep base
	int waittick;						// next tick for await
	int lasttick;						// previous tick
	int sublev;							// subroutine level
	int nestSwitch;
	LoopInfo* mem_loop;					// loop info
	SwitchInfo* mem_switch;				// switch info
	ChLongJump* mem_longjump;			// longjump-data list
	int looplev;						// repeat loop level
	RunError err;						// error code
	int chstat;							// HSP status
	int stat;							// sysvar 'stat'
	int strsize;						// sysvar 'strsize'
	char* refstr;						// RefStr Buffer
	String* refsptr;
	char* fnbuffer;						// buffer for FILENAME
	void* instance;						// Instance Handle (windows)
	int intwnd_id;						// Window ID (interrupt)
//	PVal *note_pval;					// MemNote pval
//	APTR  note_aptr;					// MemNote aptr
//	PVal *notep_pval;					// MemNote pval (previous)
//	APTR  notep_aptr;					// MemNote aptr (previous)
	char* stmp;							// String temporary buffer
	
	PrmStk* prmstk;						// Current parameter stack area
	LIBDAT* mem_linfo;					// Library info
	STRUCTPRM* mem_minfo;				// Parameter info
	STRUCTDAT* mem_finfo;				// Function/Struct info
	int retval_level;					// subroutine level (return code)
	int endcode;						// End result code
	void (*msgfunc)(HSPCTX*);			// Message Callback Proc.
	void* wnd_parent;					// Parent Window Handle
	double refdval;						// sysvar 'refdval'
	char* cmdline;						// Command Line Parameters
	
	HSPEXINFO* exinfo;					// HSP function data
	
	// clhsp
	label_t reflabel;
	ScrValue* pValResult;				// サブルーチンジャンプの返値
	int lastArgResult;					// 最後の code_get() の返値
	bool bFinInit;						// 初期化が終了した
};

#define HSPCTX_REFSTR_MAX  4096
#define HSPCTX_CMDLINE_MAX 1024

#define HSPSTAT_NORMAL 0
#define HSPSTAT_DEBUG 1
#define HSPSTAT_SSAVER 2

#define TYPE_EX_ENDOFPARAM 0x0200	// パラメーター終端(HSPtoC)
#define TYPE_EX_ARRAY_VARS 0x0201	// 配列要素付き変数用スタックタイプ(HSPtoC)

struct ChRoutine
{
	int stacklev;					// サブルーチン開始時のスタックレベル
	ushort looplev;					// サブルーチン開始時の repeat ループレベル
	ushort nestSwitch;				// 〃の nestSwitch
	short  bInExpr;					// 式中のサブルーチンか
	csptr_t lbReturn;				// 呼び出し元PCポインタ(復帰用)
	STRUCTDAT* pStDat;				// 呼び出しデータ
	PrmStk* prmstk;					// 呼び出し実引数
	PrmStk* prmstk_bak;				// 以前のスタックアドレス
};

//		コールバックのオプション
//
#define HSPEVENT_ENABLE_COMMAND 1	// １ステップ実行時
#define HSPEVENT_ENABLE_HSPIRQ 2	// HSP内での割り込み発生時
#define HSPEVENT_ENABLE_GETKEY 4	// キーチェック時
#define HSPEVENT_ENABLE_FILE 8		// ファイル入出力時
#define HSPEVENT_ENABLE_MEDIA 16	// メディア入出力時
#define HSPEVENT_ENABLE_PICLOAD 32	// picload命令実行時

struct ChTypeInfo
{
	// タイプごとの情報
	// (*の項目は、親アプリケーションで設定されます)
	short type;							// * コードタイプ値
	short option;						// * オプション情報
	HSPCTX*    ctx;						// * HSP Context構造体へのポインタ
	HSPEXINFO* exinfo;					// * HSPEXINFO構造体へのポインタ
	
	// 処理関数
	int       (* cmdfunc)( int );		// 命令処理
	ScrValue* (* reffunc)( int );		// 関数処理
	ScrValue* (* varfunc)( int );		// システム変数処理
	int       (* termfunc)( int );		// 終了時処理
	
	// イベントコールバック関数
	int (*   msgfunc)(int, int, int);			// Windowメッセージコールバック
	int (* eventfunc)(int, int, int, void*);	// HSPイベントコールバック
	
};

// 割り込みID
enum HSPIRQ
{
	HSPIRQ_ONEXIT = 0,
	HSPIRQ_ONERROR,
	HSPIRQ_ONKEY,
	HSPIRQ_ONCLICK,
	HSPIRQ_USERDEF,
	HSPIRQ_MAX
};

// HSPイベントID
enum HSPEVENT
{
	HSPEVENT_NONE = 0,
	HSPEVENT_COMMAND,
	HSPEVENT_HSPIRQ,
	HSPEVENT_GETKEY,
	HSPEVENT_STICK,
	HSPEVENT_FNAME,
	HSPEVENT_FREAD,
	HSPEVENT_FWRITE,
	HSPEVENT_FEXIST,
	HSPEVENT_FDELETE,
	HSPEVENT_FMKDIR,
	HSPEVENT_FCHDIR,
	HSPEVENT_FCOPY,
	HSPEVENT_FDIRLIST1,
	HSPEVENT_FDIRLIST2,
	HSPEVENT_GETPICSIZE,
	HSPEVENT_PICLOAD,
	HSPEVENT_MAX
};

// command extra value

// コマンドID
namespace CmdId
{
	// 比較コマンド
	typedef int Compare_t;
	namespace   Compare
	{
		static const int
			If      = 0,
			Else    = 1,
		//	Elsif   = 2,
			Switch  = 3,
			Case    = 5,
			CaseIf  = 6,
			Default = 7;
	}
	
	// 制御コマンド
	namespace Prog
	{
		static const int
			Goto    = 0x000,
			Gosub   = 0x001,
			Return  = 0x002,
			Repeat  = 0x004,
			Foreach = 0x00B,
			While   = 0xC00,
			Forever = 0xC07,
			Loop    = 0x005,
			SwEnd   = 0xC04,
			SwFall  = 0xC05,
			SwBreak = 0xC08;
	}
	
	// システム変数
	namespace Sysvar
	{
		static const int
			System      = 0x000,
			VtSysvarTop = 0x000 + Vartype::Label,
			VtSysvarMax = VtSysvarTop + Vartype::Userdef,
			
			NullPtr = 0x104,
			NullMod = 0x105,
			
			Err     = 0x200,
			LoopLev = 0x201,
			SubLev  = 0x202,
			IParam  = 0x203,
			WParam  = 0x204,
			LParam  = 0x205,
			StrSize = 0x206,
			Cnt     = 0x207,
			SwThis  = 0x208,
			Thismod = 0x209,
			
			ChStat  = 0x300,
			ChVer   = 0x301;
	}
	
	// 宣言
	namespace Declare
	{
		static const int
			Var       = 0x000,
			StaticVar = 0x001,
			GlobalVar = 0x002,
			Const     = 0x003,
			LitId     = 0x004,
			Enum      = 0x005,
			Alias     = 0x006,
			Function  = 0x007,
			
			ConstExpr = 0x100;
	}
}

#endif
