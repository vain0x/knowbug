// hsedsdk for C++ (ver 3.31RC1)

#include "hsedsdk.h"

static HANDLE hReadPipe;
static HANDLE hWritePipe;
static HANDLE hDupReadPipe;
static HANDLE hDupWritePipe;

static HWND hIF;

//
// パイプ ハンドルの解放
void hsed_uninitduppipe()
{
	if ( hReadPipe )     { CloseHandle( hReadPipe );     hReadPipe     = 0; }
	if ( hWritePipe )    { CloseHandle( hWritePipe );    hWritePipe    = 0; }
	
	if ( hDupReadPipe )  { CloseHandle( hDupReadPipe );  hDupReadPipe  = 0; }
	if ( hDupWritePipe ) { CloseHandle( hDupWritePipe ); hDupWritePipe = 0; }
	return;
}

//
// パイプ ハンドルの作成
bool hsed_initduppipe( int nSize )
{
	CreatePipe( &hReadPipe, &hWritePipe, 0, nSize );
	if ( hReadPipe == 0 || hWritePipe == 0 ) return true;

	DWORD dwProcessId;
	GetWindowThreadProcessId( hIF, &dwProcessId );
	HANDLE const hHsedProc = OpenProcess( PROCESS_ALL_ACCESS, 0, dwProcessId );

	HANDLE const hCurProc = GetCurrentProcess();

	DuplicateHandle( hCurProc, hReadPipe,  hHsedProc, &hDupReadPipe,  0, 0, DUPLICATE_SAME_ACCESS );
	DuplicateHandle( hCurProc, hWritePipe, hHsedProc, &hDupWritePipe, 0, 0, DUPLICATE_SAME_ACCESS );

	CloseHandle( hHsedProc );
	if ( hDupReadPipe == 0 || hDupWritePipe == 0 ) { hsed_uninitduppipe(); return false; }
	return false;
}

//
// スクリプト エディタのAPIウィンドウの捕捉
bool hsed_capture()
{
	hIF = FindWindow( HSED_INTERFACE_NAME, HSED_INTERFACE_NAME );
	return ( hIF == 0 );
}

//
// スクリプト エディタが起動しているかチェック
bool hsed_exist()
{
	return !hsed_capture();
}

/*
//
// スクリプト エディタのバージョンを取得
int hsed_getver( var ret, int nType )
{
	if ( hsed_capture() ) {
		ret = 0; return 1;
	}
	
	if ( nType == HGV_HSPCMPVER ) {
		if ( hsed_initduppipe( 4096 ) ) return 2;
		
		if ( SendMessage( hIF, _HSED_GETVER, nType, hDupWritePipe ) < 0 ) {
			ret = "Error";
			hsed_uninitduppipe();
			return 3;
		}
		
		if ( PeekNamedPipe( hReadPipe, 0, 0, 0, dwTotalBytesAvail, 0 ) == 0 ) {
			hsed_uninitduppipe();
			return 4;
		}
		
		sdim ret, dwTotalBytesAvail + 1
		if ( dwTotalBytesAvail > 0 ) {
			ReadFile( hReadPipe, ret, dwTotalBytesAvail, dwNumberOfBytesRead, 0 );
		}
		hsed_uninitduppipe();

	} else {
		ret = SendMessage( hIF, _HSED_GETVER, nType, 0 );
		if ( ret < 0 ) return 3;
	}
	return 0;
}

//
// スクリプト エディタの各種ハンドルを取得
int hsed_getwnd( HWND& ret, int nType, int nID )
{
	if ( hsed_capture() ) { ret = 0; return 1; }

	ret = ( nType == HGW_EDIT )
		? SendMessage( hIF, _HSED_GETWND, nType, nID )
		: SendMessage( hIF, _HSED_GETWND, nType, 0 );
	
	return ( ret == 0 ) ? 2 : 0;
}
//*/

//
// ファイルパスを取得
int hsed_getpath( char* ret, int nTabID )
{
	if ( hsed_capture() ) return 1;
	if ( hsed_initduppipe( 260 ) ) return 2;

	if ( SendMessage( hIF, _HSED_GETPATH, nTabID, (LPARAM)hDupWritePipe ) < 0 ) {
		strcpy( ret, "Error" );
		hsed_uninitduppipe();
		return 3;
	}
	
	DWORD dwTotalBytesAvail;
	if ( PeekNamedPipe( hReadPipe, 0, 0, 0, &dwTotalBytesAvail, 0 ) == 0 ) {
		hsed_uninitduppipe();
		return 4;
	}
	
	if ( dwTotalBytesAvail > 0 ) {
		DWORD dwNumberOfBytesRead;
		ReadFile( hReadPipe, ret, dwTotalBytesAvail, &dwNumberOfBytesRead, 0 );
	}
	hsed_uninitduppipe();
	return 0;
}

/*
//
// バージョンの数値を文字列に変換
void hsed_cnvverstr( int nVersion )
{
	int beta = hsed_getbetaver(nVersion);
	
	sdim _refstr, 4096
	if ( beta ) {
		sprintf( _refstr, "%d.%02db%d", hsed_getmajorver(nVersion), hsed_getminorver(nVersion), beta );
	} else {
		sprintf( _refstr, "%d.%02d", hsed_getmajorver(nVersion), hsed_getminorver(nVersion) );
	}
	
	return _refstr;
}
//*/

//
// タブ数の取得
int hsed_gettabcount( int& ret )
{
	if ( hsed_capture() ) { ret = -1; return 1; }

	ret = SendMessage( hIF, _HSED_GETTABCOUNT, 0, 0 );
	return 0;
}

/*
//
// FootyのIDからタブのIDを取得
void hsed_gettabid( int& ret, int nFootyID )
{
	if ( hsed_capture() ) { ret = -1; return 1; }

	ret = SendMessage( hIF, _HSED_GETTABID, nFootyID );
	return 0;
}

//
// タブのIDからFootyのIDを取得
void hsed_getfootyid(var ret, int nTabID)
{
	if ( hsed_capture() ) { ret = -1; return 1; }

	SendMessage hIF, _HSED_GETFOOTYID, nTabID
	if ret < 0{
		ret = -1
		return 2
	} else {
		ret = stat
		return 0
	}
}

//
// コピーの可否を取得
void hsed_cancopy(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_CANCOPY, nFootyID
	ret = stat
	if ( ret < 0 ) { return 2: else: return 0
}

//
// 貼り付けの可否を取得
void hsed_canpaste(var ret)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_CANPASTE
	ret = stat
	return 0
}

//
// アンドゥの可否を取得
void hsed_canundo(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_CANUNDO, nFootyID
	ret = stat
	if ( ret < 0 ) { return 2: else: return 0
}

//
// リドゥの可否を取得
void hsed_canredo(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_CANREDO, nFootyID
	ret = stat
	if ( ret < 0 ) { return 2: else: return 0
}

//
// 変更フラグを取得
void hsed_getmodify(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_GETMODIFY, nFootyID
	ret = stat
	if ( ret < 0 ) { return 2: else: return 0
}

//
// コピー
void hsed_copy(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_COPY, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// 切り取り
void hsed_cut(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_CUT, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// 貼り付け
void hsed_paste(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_PASTE, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// アンドゥ
void hsed_undo(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_UNDO, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// リドゥ
void hsed_redo(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_REDO, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// インデント
void hsed_indent(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_INDENT, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// アンインデント
void hsed_unindent(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_UNINDENT, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// すべて選択
void hsed_selectall(int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_SELECTALL, nFootyID
	if ( stat == -1 ) { return 0: else: return
}

//
// 文字列長を取得
void hsed_gettextlength(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_GETTEXTLENGTH, nFootyID
	if ( stat < 0 ) { return 1: else: ret = stat: return 0
}

//
// 行数を取得
void hsed_getlines(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_GETLINES, nFootyID
	if ( stat < 0 ) { return 1: else: ret = stat: return 0
}

//
// 行の文字列長を取得
void hsed_getlinelength(var ret, int nFootyID, int nLine)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_GETLINELENGTH, nFootyID, nLine
	if ( stat < 0 ) { return 1: else: ret = stat: return 0
}

//
// 改行コードの取得
void hsed_getlinecode(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	SendMessage hIF, _HSED_GETLINECODE, nFootyID
	if stat == -5{
		ret = -1
		return 1
	} else {
		ret = stat
		return 0
	}
}

//
// 文字列の取得
void hsed_gettext(var ret, int nFootyID)
{
	hsed_capture
	if ( stat ) { return 1

	hsed_gettextlength nLength, nFootyID
	if ( stat ) { return 2

	sdim ret, nLength + 1
	if ( nLength == 0  ) { return 0
	hsed_initduppipe nLength + 1
	if ( stat ) { return 3

	SendMessage hIF, _HSED_GETTEXT, nFootyID, hDupWritePipe
	if ( stat < 0 ) { ret = "Error": hsed_uninitduppipe: return 4

	ReadFile hReadPipe, ret, nLength, dwNumberOfBytesRead, 0
	hsed_uninitduppipe
	return 0
}

void hsed_sendtext_msg(int nFootyID, int msg, var sText)
{
	hsed_capture
	if ( stat ) { return 1

	nLength = strlen(sText)

	hsed_initduppipe nLength + 1
	if ( stat ) { return 3

	WriteFile hWritePipe, sText, nLength + 1, dwNumberOfBytesWritten, 0

	SendMessage hIF, msg, nFootyID, hDupReadPipe
	if ( stat < 0 ) { hsed_uninitduppipe: return 4
	
	hsed_uninitduppipe
	return 0
}

void hsed_settext(int nFootyID, str sText)
{
	vText = sText
	hsed_sendtext_msg nFootyID, _HSED_SETTEXT, vText
	sdim vText
	return
}

//
// アクティブなFootyのIDの取得
void hsed_getactfootyid(var ret)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_GETACTFOOTYID
	ret = stat
	return 0
}

//
// アクティブなタブのIDの取得
void hsed_getacttabid(var ret)
{
	hsed_capture
	if ( stat ) { ret = -1: return 1

	SendMessage hIF, _HSED_GETACTTABID
	ret = stat
	return 0
}

//
// 指定された文字列をエディタに送る
void hsed_sendstr(var _p1)
{
	hsed_getactfootyid actid
	if ( stat  ) { return 1

	vinfo=sysinfo(0)
	if ( instr(vinfo,0,"WindowsNT")<0  ) { goto *sendbyclip
	vdbl=0.0+strmid(vinfo,13,8)
	if ( vdbl<5.1  ) { goto *sendbyclip

	;	直接文字列データを送信する
	hsed_sendtext_msg actid, _HSED_SETSELTEXT, _p1
	return
*sendbyclip
	;	クリップボード経由で文字列を送信する
	;	(WindowsXPより前の環境用)
	OpenClipboard
	ret=stat : if ( ret!0  ) { EmptyClipboard

	;クリップボードにテキストデータを設定
	ls=strlen(_p1)+1
	lngHwnd=GlobalAlloc(2,ls)
	if ( lngHwnd!0 ) {
		lngMem=GlobalLock(lngHwnd)
		if ( lngMem!0 ) {
			ret=lstrcpy(lngMem,varptr(_p1))
			if ( ret!0 ) {
				SetClipboardData CF_OEMTEXT,lngHwnd
			}
			GlobalUnlock lngHwnd : lngRet=stat
		}
	}
	CloseClipboard
	SendMessage hIF, _HSED_PASTE,-1, 0
	return
}
//*/
