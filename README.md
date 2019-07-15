# knowbug

[![Build status](https://ci.appveyor.com/api/projects/status/67ue70udoicrb98v?svg=true)](https://ci.appveyor.com/project/vain0x/knowbug)

[最新版はこちら](https://github.com/vain0x/knowbug/releases/latest)

## 概要

**knowbug** は、HSP3 用デバッグ・ウィンドウの非公式改造版です。

![スクリーンショット](./package/screenshot/static_variables.png)

主に「変数」タブの機能を拡張しており、以下のものの表示に対応しています。

* int の16進数表現の併記
* 多次元配列の2次元全要素
* モジュール変数のメンバ変数
* 実行中のユーザ定義コマンドの実引数

**[機能の詳細はこちらを参照](details.md)** してください。

## インストール

#### knowbug 本体

0. [最新版のパッケージ](https://github.com/vain0x/knowbug/releases/latest) の `knowbug-package.zip` をダウンロードします。
0. HSPのフォルダ(※)にある ``hsp3debug.dll`` と ``hsp3debug_64.dll`` の名前を変更します。
  * 例:``hsp3debug.dll`` → ``hsp3debug__default.dll``
0. `package` フォルダにある ``hsp3debug_knowbug.dll`` の名前を ``hsp3debug.dll`` に変えて、HSPのフォルダに移動します。

※「HSPのフォルダ」の位置は、HSPで ``path = dir_exe : input path, ginfo_winx`` を実行すると分かります。

#### WrapCall

(省略可)

WrapCall プラグインを追加すると、ユーザー定義コマンドに関する機能が有効になります。

* 同梱されている ``WrapCall.as`` と ``userdef.as`` をHSPの `common` フォルダに移動します。
  * 既に ``userdef.as`` がある場合は、上書きではなく追記してください。

**注意**: 64bit版ランタイムを使う際は、標準のヘッダ ``hsp3_64.as`` を WrapCall より前に \#include しておく必要があります。

#### 設定ファイル

(省略可)

[設定ファイル](./package/knowbug.ini) を knowbug (hsp3debug.dll) と同じフォルダに置いておくと、起動時に読み込まれます。

* 具体的な設定については、設定ファイル内のコメントを参照してください。

## アンインストール

アンインストールするには、インストールとは逆の操作を行います。

0. knowbug (``hsp3debug.dll`` と ``hsp3debug_64.dll``) を削除し、バックアップしておいた、元々の ``hsp3debug.dll`` を、元に戻します。
0. 設定ファイル (``knowbug.ini``) とヘッダーファイル (``WrapCall.as``) は不要なので削除します。``userdef.as`` を元に戻します。

## 推奨環境

- HSP 3.5.1
- Windows 10

## 不具合報告など

既知の不具合や要望などは以下にまとまっています。不具合の報告やご意見などは、ここに書いてもらえると嬉しいです。

- [Issues](https://github.com/vain0x/knowbug/issues)

knowbug のリリースの一覧は以下を参照してください。(pre-release は動作が安定していない可能性があります。)

- [Releases](https://github.com/vain0x/knowbug/releases)

## 関連リンク

- OpenHSP <http://dev.onionsoft.net/trac>
- GINGER <https://github.com/vain0x/hsp3-debug-ginger>
