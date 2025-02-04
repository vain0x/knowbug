# 開発環境

## 環境

OS: Windows 10

以下をインストールしていると仮定します

- [Git for Windows](https://gitforwindows.org/)
- [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest) (>= 7)
- [Visual Studio 2022 Community](https://visualstudio.microsoft.com/vs)
    - C++ 開発用の機能をインストールしておく

## ソリューション

ソリューション (`src/knowbug.sln`) を Visual Studio で開いて「ビルド」(Ctrl+Shift+B)します。

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
