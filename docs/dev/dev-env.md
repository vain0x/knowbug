# 開発環境

ここでは開発環境の前提条件、ビルド手順、ビルド設定の説明などを記述しています

- knowbug は **サーバー** (= dll, C++) と **クライアント** (= exe, HSP) の2つのプログラムから構成されています
    - 詳しくは [architecture.md](architecture.md) を参照してください
- ビルド手順は IDE (Visual Studio) を使う方法と、ビルドスクリプトを使う方法の2通りがあります

## 開発環境の前提条件

- OS: Windows 10/11

以下をインストールしてください

- [Visual Studio 2022 Community](https://visualstudio.microsoft.com/vs)
    - Visual Studio Installer で「C++ によるデスクトップ開発」を選択
- [Git for Windows](https://gitforwindows.org/) (必要なら)

## ビルド手順 (簡易)

### サーバー側

- Visual Studio でソリューション (`src/knowbug.sln`) を開く
    - メニューバーの「ビルド」→「ソリューションをビルド」を行う
    - ビルドに成功すれば、ビルド出力が `src/knowbug_dll/bin/Debug/Win32/hsp3debug.dll` (など) に生成されます
    - dllをHSPのインストールディレクトリに置くと、デバッグウィンドウの表示時にロードされます

### クライアント側

ソースコードが UTF-8 エンコーディングなので、標準のスクリプトエディタではビルドできません

簡単なのは [hsed3s](https://hsp.moe/hsed3s/alpha1/) を使うことです

- `src/knowbug_client/kc_main.hsp` を開いて実行ファイルを作成します
    - 作成に成功すれば、`knowbug_client.exe` が生成されます
    - exeをHSPのインストールディレクトリに置くと、(knowbugのdllもインストールされていれば) デバッグウィンドウの表示時に自動で実行されます

また、HSPのサンプルにある `sample/misc/mkexe.hsp` でも可能なはずです

## ビルド設定

### ソリューション

- コンフィギュレーション
    - Debug/Release
        - Debug は knowbug 自体のデバッグ用。
        - Release は配布用。
        - shift_jis ランタイム用
    - DebugUtf8/ReleaseUtf8
        - UTF-8 ランタイム用
- プラットフォーム
    - x86/x64
        - **x86**: 32 ビット版
        - **x64**: 64 ビット版 (`hsp3debug_64.dll`)
- 出力
    - `src/*/bin/*/*/hsp3debug*.dll`

## テスト

knowbug_tests プロジェクトを起動するとテストが実行され、一定の動作確認を行えます。(ただしテストコードは少ないです。)

## 動作確認

`./sandbox` のサンプルコードなどを使って動作確認を行います。

- スクリプトの実行中に Visual Studio の「プロセスにアタッチ」(Ctrl+Alt+P)で hsp3.exe を選ぶと knowbug 側のコードをトレース実行できて便利です。
- knowbug の起動時にシフトキーを押しておくと、knowbug の開始時に停止するようになっていて、アタッチしやすくなります。

## ビルドスクリプト

`./scripts` にあるビルドスクリプトを使う場合は、以下の通り、一定の設定が必要です。

- 開発用に HSP3 をインストールしてください。
    - `./scripts/dev-install-hsp3` で自動的にインストールできるはずです。
    - bin/server と bin/client に配置されます。

## デバッグ版のインストール

ビルドで生成される DLL を指すシンボリックリンクをインストールしておくと便利です。

シンボリックリンクを手動で作成するのはめんどうなので、スクリプトを用意しています。管理者用の PowerShell (バージョン 7 以上の方) を開き、以下のスクリプトを実行してください。

```pwsh
./scripts/dev-install-link.ps1
```

インストール先の環境でスクリプトをデバッグ実行するには `./scripts/run.ps1` が使用できます。`./scripts/run-default.ps1` は `./sandbox/default.hsp` を実行します。

```pwsh
./scripts/run-default.ps1
```
