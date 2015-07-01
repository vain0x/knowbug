// struct ModcmdCallInfo

#ifndef IG_STRUCT_MODCMD_CALL_INFO_H
#define IG_STRUCT_MODCMD_CALL_INFO_H

struct STRUCTDAT;

// 構造体定義 (呼び出し直前の情報)
struct ModcmdCallInfo
{
	int  sublev;				// サブルーチン呼び出し直前のレベル
	int looplev;
	
	const char *fname;		// 呼び出し位置のファイル名
	int line;				// 呼び出した行
	
	STRUCTDAT* pStDat;		// 呼ばれたコマンドの情報
	void* prmstk_bak;		// 1つ前に呼ばれたコマンドの実引数データ
	
	// 連結リスト
	const ModcmdCallInfo* prev;
	const ModcmdCallInfo* next;
	
public:
	bool isRunning() const {	// 実行中か (引数展開が終了しているか)
		return next == nullptr || sublev < next->sublev;
	}
};

#endif
