<<<<<<< f706ef994a92adc87d60e57079bd8f195501f2a9
﻿# knowbug
* version: 1.22

## 概要
HSP3 用デバッグ・ウィンドウの非公式改造版です。

![スクリーンショット](./package/screenshot/static_variables.png)

主に「変数」タブの機能を拡張しており、以下のものの表示に対応しています。

* int の16進数表現の併記
* 多次元配列の2次元全要素
* モジュール変数のメンバ変数
* 実行中のユーザ定義コマンドの実引数

**[機能の詳細はこちらを参照](details.md)**してください。

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
knowbug は、公式のデバッガ hsp3debug.dll に、ue_dai が手を加えたものです。

本ソフトの半分程度は、[OpenHSP にあるコード、およびリソース](http://dev.onionsoft.net/trac/openhsp/browser/trunk/tools/win32/hsp3debug)を使用しています。[ライセンス](./package/License/License_j.txt)も参照してください。

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
=======
# tinytoml

A header only C++11 library for parsing TOML.

[![Build Status](https://travis-ci.org/mayah/tinytoml.svg?branch=master)](https://travis-ci.org/mayah/tinytoml)

This parser is based on TOML [v0.4.0](https://github.com/toml-lang/toml/blob/master/versions/en/toml-v0.4.0.md).
This library is distributed under simplified BSD License.

## Introduction

tinytoml is a tiny [TOML](https://github.com/toml-lang/toml) parser for C++11 with following properties:
- header file only
- C++11 library friendly (array is std::vector, table is std::map, time is std::chrono::system_clock::time_point).
- no external dependencies (note: we're using cmake for testing, but it's not required to use this library).

We'd like to keep this library as handy as possible.

## Prerequisite

- C++11 compiler
- C++11 libraries.

I've only checked this library works with recent clang++ (3.5) and g++ (4.7). I didn't check this with cl.exe.
Acutally I'm using this library on my Linux app and Mac app. However, I haven't written Windows app yet.

## How to use

Copy include/toml/toml.h into your project, and include it from your source. That's all.

## Example code

```c++
std::ifstream ifs("foo.toml");
toml::Parser parser(ifs);

toml::Value v = parser.parse();
// If toml file is valid, v would be valid.
// Otherwise, you can get an error reason by calling Parser::errorReason().
if (!v.valid()) {
    cout << parser.errorReason() << endl;
    return;
}

// You can find a value by find().
// If found, non-null pointer will be returned.
// You can check the type of value with is().
// You can get the inner value by as().
const toml::Value* x = v.find("bar");
if (x && x->is<std::string>()) {
    cout << x->as<string>() << endl;
} else if (x && x->is<int>()) {
    cout << x->as<int>() << endl;
}

// Note: the inner value of integer value is actually int64_t,
// however, you can use 'int' for convenience.
toml::Value* z = ...;
int x = z->as<int>();
int y = z->as<int64_t>();
// toml::Array is actually std::vector<toml::Value>.
// So, you can use range based for, etc.
const toml::Array& ar = z->as<toml::Array>();
for (const toml::Value& v : ar) {
    ...
}

// For convenience way, you can use get() when you're sure that the value exists.
// If type error occurred, std::runtime_error is raised.
toml::Value v = ...;
cout << v.get<string>("foo.bar") << endl;
```

## How to test

The directory 'src' contains a few tests. We're using google testing framework, and cmake.

```sh
$ mkdir -p out/Debug; cd out/Debug
$ cmake ../../src
$ make
$ make test
```

'src' also contains a small example for how to use this.
>>>>>>> Squashed 'src/tinytoml/' content from commit b0d1b1c
