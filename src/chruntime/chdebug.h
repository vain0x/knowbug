// clhsp - declaration for debug

#ifndef IG_CLHSP_DEBUG_INFO_H
#define IG_CLHSP_DEBUG_INFO_H

struct HSPCTX;
typedef       void*   valptr_t;
typedef const void* c_valptr_t;
typedef short vartype_t;

// 実行時エラーコード
enum RunError
{
	RunErr_Top  = 0,
	RunErr_None = RunErr_Top,
	RunErr_UnknownCode,				// システムエラーが発生しました
	RunErr_Syntax,					// 文法が間違っています
	RunErr_IllegalArg,				// パラメータの値が異常です
	RunErr_Expr,					// 計算式でエラーが発生しました
	RunErr_NoDefault,				// パラメータの省略はできません
	RunErr_VtMisMatch,				// パラメータの型が違います
	RunErr_InvalidArrIdx,			// 配列の要素が無効です
	RunErr_LabelRequired,			// 有効なラベルが指定されていません
	RunErr_TooManyNesting,			// サブルーチンやループのネストが深すぎます
	RunErr_InvalidReturnSttm,		// サブルーチン外のreturnは無効です
	RunErr_InvalidLoopSttm,			// repeat外でのloopは無効です
	RunErr_FileMissed,				// ファイルが見つからないか無効な名前です
	RunErr_FileMissed__,
	RunErr_ExternalExec,			// 外部ファイル呼び出し中のエラーです
	RunErr_Priority,				// 計算式でカッコの記述が違います *
	RunErr_TooManyArgs,				// パラメータの数が多すぎます
	RunErr_TooBigData,				// 文字列式で扱える文字数を越えました *
	RunErr_WrongSttmCmd,			// 代入できない変数名を指定しています
	RunErr_DivisionByZero,			// 0で除算しました
	RunErr_BufferOverflow,			// バッファオーバーフローが発生しました"
	RunErr_Unsupported,				// サポートされない機能を選択しました
	RunErr_TooManyBrackets,			// 計算式のカッコが深すぎます *
	RunErr_VariableRequired,		// 変数名が指定されていません
	RunErr_LValRequired = RunErr_VariableRequired,
	RunErr_IntRequired,				// 整数以外が指定されています
	RunErr_SyntaxArray,				// 配列の要素書式が間違っています
	RunErr_MemOut,					// メモリの確保ができませんでした
	RunErr_VtInitFailed,			// タイプの初期化に失敗しました		
	RunErr_NoFunctionArgs,			// 関数に引数が設定されていません *
	RunErr_StackOverflow,			// スタック領域のオーバーフローです		
	RunErr_InvalidPrm,				// 無効な名前がパラメータに指定されている
	RunErr_InvalidArrayStore,		// 配列変数に異なる型の値を代入した
	RunErr_InvalidFuncPrm,			// 関数のパラメーター記述が不正です
	RunErr_TooManyWindowObject,		// オブジェクト数が多すぎます
	RunErr_NoArray,					// 配列・関数として使用できない型です
	RunErr_ModinstRequired,			// モジュール変数が指定されていません
	RunErr_InvalidModcls,			// モジュール変数の指定が無効です
	RunErr_InvalidVtCnv,			// 変数型の変換に失敗しました
	RunErr_UsingExternalDll,		// 外部DLLの呼び出しに失敗しました
	RunErr_UsingComobj,				// 外部オブジェクトの呼び出しに失敗しました
	RunErr_NoResult,				// 関数の戻り値が設定されていません
	RunErr_FunctionSyntax,			// 関数を命令として記述しています
	RunErr_SmArglistUnused,			// レシーバが使用されていません
	RunErr_UsingVoidValue,			// void の値は使用できません
	RunErr_UsingReserved,			// 予約語は使用できません
	RunErr_ModclsMisMatch,			// モジュールクラスが違います
	RunErr_Unknown,					// 謎のエラー
	RunErr_UnusablePolyArray,		// 多次元配列は使用できません
	RunErr_UnusableNullValue,		// null 値は使用できません
	RunErr_UnusableClone,			// clone は使用できません
	RunErr_UsingUnPackedValue,		// unpack により展開された値は演算に使用できません
	RunErr_,
	RunErr_IrqJump,					// 割り込みジャンプ時
	RunErr_ExitRun,					// 外部ファイル実行
	RunErr_Max
};

// Debug Info ID
enum DebugInfoId
{
	DebugInfoId_General = 0,
	DebugInfoId_VarName,
	DebugInfoId_IntInfo,
	DebugInfoId_GrInfo,
	DebugInfoId_MMInfo,
	DebugInfoId_Max
};
typedef DebugInfoId DEBUGINFO;

// Debug Flag ID
enum DebugAction
{
	DebugAction_None = 0,
	DebugAction_Run,			// 実行再開
	DebugAction_Stop,			// 停止
	DebugAction_StepIn,			// 次の一行を処理
	DebugAction_StepOver,		// 次の一行を処理、サブルーチンジャンプは飛ばす ( 未実装 )
	DebugAction_Max
};

// デバッグウィンドウへの通知ID
enum DebugNotice
{
	DebugNotice_None = 0,
	DebugNotice_Stop = 0,		// 停止したとき ( stop or assert )
	DebugNotice_Logmes,			// logmes 命令が実行されたとき ( ctx->stmp に文字列 )
	DebugNotice_Max
};

struct DebugInfo
{
	//	[in] system value
	//	(初期化後に設定されます)
	//
	HSPCTX* ctx;
	
	char* (* get_value) ( int );			// debug情報取得コールバック
	char* (* get_varinf)( char*, int );		// 変数情報取得コールバック
	void  (* dbg_close) ( char* );			// debug情報取得終了
	void  (* dbg_curinf)(void);				// 現在行・ファイル名の取得
	int   (* dbg_set)   ( int );			// debugモード設定
	
	char* (* dbg_toString)( c_valptr_t, vartype_t );	// デバッグ文字列の取得
	
	//	[in/out] tranfer value
	//	(システムとの通信用)
	
	int	flag;				// Flag ID
	int	line;				// 行番号情報
	char* fname;			// ファイル名情報
	void* dbgwin;			// Debug WindowのHandle
	char* dbgval;			// debug情報取得バッファ
};

//##############################################################################
//                グローバル関数
//##############################################################################
extern const char* hspd_geterror( RunError err );

#endif
