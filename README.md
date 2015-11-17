# knowbug v1.22
HSP3 用デバッグ・ウィンドウの非公式改造版です。

![スクリーンショット](./package/screenshot/static_variables.png)

主に「変数」タブの機能を拡張しており、以下のものの表示に対応しています。

* int の16進数表現の併記
* 多次元配列の2次元全要素
* モジュール変数のメンバ変数
* 実行中のユーザ定義コマンドの実引数

**[機能の詳細はこちらを参照](detail.md)**してください。

## 導入方法
#### knowbug 本体
0. [最新版のパッケージ](https://github.com/vain0/knowbug/releases/latest) の `knowbug-package.zip` をダウンロードします。
0. HSPのフォルダにある「hsp3debug.dll」の名前を変更します。
0. [package フォルダ](./package)にある「hsp3debug_knowbug.dll」の名前を「hsp3debug.dll」に変えて、HSPのフォルダに移動します。

* 「HSPのフォルダ」の正確な位置は、HSPスクリプトエディタで ``mes dir_exe`` を実行すると分かります。
* 64bit版を使うには、「hsp3debug_64_knowbug.dll」も「hsp3debug_64.dll」という名前にして、HSPのフォルダに移動します。

#### WrapCall
0. 同梱されている「WrapCall.as」と「userdef.as」を common フォルダに移動します。
  * 既に userdef.as を使っている方は、上書きではなく追記してください。

* WrapCall はユーザ定義コマンドに関する機能に用いられます。無くても正常に動作します。
* 64bit版ランタイムを使う際は、標準のヘッダ ``hsp3_64.as`` を WrapCall より前に \#include しておく必要があります。

#### 設定ファイル
[設定ファイル](./package/knowbug.ini) を knowbug (hsp3debug.dll) と同じフォルダに置いておくと、起動時に読み込まれます。

* 具体的な設定については、設定ファイル内のコメントを参照してください。
* 無くても正常に動作します。

## 除去方法
「導入方法」と逆の操作をします。なおレジストリなどにデータは残りません。

0. knowbug (ファイル名 hsp3debug.dll) を削除し、バックアップしておいた、元々の
hsp3debug.dll を、元に戻します。
0. 「knowbug.ini」「WrapCall.as」は不要なので削除します。

## 動作環境
* HSP3.5 以降
* OS: Windows 7

これら以外では、動作を確認していません。サポートできない可能性も高いのでご了承ください。

## 権利
knowbug は、公式のデバッガ hsp3debug.dll に、uedai が手を加えたものです。

本ソフトの半分程度は、[OpenHSP にあるコード、およびリソース](http://dev.onionsoft.net/trac/openhsp/browser/trunk/tools/win32/hsp3debug)を使用しています。[ライセンス](./package/Lisense/License_j.txt)も参照してください。

## 関連URL
バグ報告、意見、要望などは、[GitHub の Issue 表](https://github.com/vain0/knowbug/issues) または [プログラ広場の掲示板](http://uedai-kami.bbs.fc2.com/) までお願いします。

* knowbug on GitHub <https://github.com/vain0/knowbug/>:
  * knowbug のソースコードをアップロードしています。
  * [リリース一覧](https://github.com/vain0/knowbug/releases)
  * [既知の不具合の一覧](https://github.com/vain0/knowbug/labels/bug)

* HSPTV! <http://hsp.tv/>:
  * HSP3 の公式サイトです。
* OpenHSP <http://dev.onionsoft.net/trac/>:
  * プロジェクト OpenHSP のリポジトリがあるサイトです。
  * HSPはここで開発されています。
