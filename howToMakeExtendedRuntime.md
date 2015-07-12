
* プラグインが作れる人限定。

* hsp3 を適当な位置にコピーする。
* ./win3gui/hsp3.vcproj (コピーの方) をVC++で開く (2008でのみ確認)
* プロジェクトに次の3つのファイルのコピーを加える。
  * hsp3plugin.h と、
  * プラグインを登録する関数を宣言した .h (rtmain.h  と仮定)
  * cmdfunc などの実装があるファイル .cpp (rtmain.cppと仮定)
* hsp3plugin.h から extern 付きの変数宣言を削除する。
 * hsp3plugin.cpp にあるグローバル変数宣言を rtmain.cpp の上部にコピーし、すべて static にする。
* hsp3plugin.cpp の関数宣言を rtmain.cpp の上部に、実装を下の方に、それぞれコピーする。
  * hsp3plugin.cpp はプロジェクトから外す。
* hsp3code.cpp に、加筆修整。
  * ``#include "rtmain.h"``
    * ファイル上部の #include 群の最後あたりに。
  * ``hsp3typeinit_**( code_gettypeinfo(-1) );``
    * 2420行目あたりの、code_init 関数のなかにある、hsp3typeinit 群に名を連ねる。
    * 関数名は rtmain.h のもの
* ビルド。
* hsp のインストールされているフォルダに、できあがったランタイム .exe を移動。
* 次のような as ファイルを作って #include すればできあがり。
```hsp
#runtime "ランタイム名( 拡張子除く )"
#regcmd 18
#cmd ...	// キーワードを定義
```

* プラグインを登録する関数 … hsp3sdk_init を呼び出している関数。
