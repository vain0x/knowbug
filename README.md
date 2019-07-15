# knowbug

[![Build status](https://ci.appveyor.com/api/projects/status/67ue70udoicrb98v?svg=true)](https://ci.appveyor.com/project/vain0x/knowbug)

* version: 1.22.2

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

0. [最新版のパッケージ](https://github.com/vain0/knowbug/releases/latest) の `knowbug-package.zip` をダウンロードします。
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

## 動作環境

* HSP 3.5
* Windows 7

これら以外では、動作を確認していません。サポートできない可能性も高いのでご了承ください。

## ライセンス

knowbug は、公式のデバッグウィンドウに、 @vain0 が手を加えたものです。

本ソフトの一部として、[OpenHSP にあるコード、およびリソース](http://dev.onionsoft.net/trac/openhsp/browser/trunk/tools/win32/hsp3debug)を使用しています。[ライセンス](./package/License/License_j.txt)も参照してください。

@vain0 が権利を有する部分はパブリックドメイン扱いとします。

## 関連URL

バグ報告、意見、要望などは、[GitHub の Issue 表](https://github.com/vain0/knowbug/issues) または [プログラ広場の掲示板](http://uedai-kami.bbs.fc2.com/) までお願いします。

* knowbug のソースコード <https://github.com/vain0/knowbug/>:
  * [既知の不具合の一覧](https://github.com/vain0/knowbug/labels/bug)
  * [リリース一覧](https://github.com/vain0/knowbug/releases)
* OpenHSP <http://dev.onionsoft.net/trac/>:
  * プロジェクト OpenHSP の公式サイトです。HSP本体やスクリプトエディター、デバッグウィンドウなどのソースコードがあります。
  * knowbug のソースコードの一部をこちらから流用しています。
