# WrapCall
## 概要
TYPE_MODCMD タイプのコマンド処理をラップすることで、ユーザ定義命令や関数の呼び出しを DebugWindow (Knowbug) に通知する。

* かつては exinfo->er を破壊的に使用していた。
  * HSP2時代のプラグインは exinfo->er を使用する。それらと併用するには、knowbug の比較的新しいバージョンを使用しなければならない。

## 仕組み
* HSP3TYPEINFO がネイティブ配列 (HSP3TYPEINFO[]) で管理されていることを前提とする。
* ラッピングは最後まで解かない。
  * HSP3TYPEINFO が realloc されると、見失ってしまうため。

## 更新履歴
#### 2014.08/20
* knowbug 本体と統合した。
  * これまでは WrapCall.hpi という独立した HPI だった。

#### 2014.02/14
* knowbug の公開APIから hwnd を取り出して通信するようにした。
  * exinfo->er は使用しなくなった。

#### 2012.09/10
* ModcmdCallInfo を双方向リストにした。
* 返値が返却されたときに通知 ResultReturning を送るようにした。
* 終了時に、ModcmdCallInfo をすべて削除するようにした。
  * 途中で終了したとき、それらを delete することなく終了していた。

#### 2012.05/27
* 返値 (RUNMODE) を、本来の返値が RUNMODE_RUN であれば、代わりに knowbug から受け取った値を返却するようにした。
  * この機能は使用していない。
