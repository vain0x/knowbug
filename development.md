# knowbug 開発者用メモ

## 開発環境

- Windows 10
- [Git for Windows](https://gitforwindows.org/)
- [PowerShell 6](https://github.com/PowerShell/PowerShell/releases/latest)
- [Visual Studio 2019 Community](https://visualstudio.microsoft.com/vs)
    - C++ 開発用の機能をインストールしておく。

## ソリューション

ソリューション (`src/knowbug.sln`) を Visual Studio で開いて「ビルド」(Ctrl+Shift+B)します。

- コンフィギュレーション
    - Debug/Release
        - Debug は knowbug 自体のデバッグ用。
        - Release は配布用。
        - shift_jis ランタイム用
    - DebugUtf8/ReleseUtf8
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

## 環境変数

`./scripts` にあるビルドスクリプトを使うには、以下の環境変数が必要です。(Win+Break → システムの詳細設定 → 環境変数 → 新規)

- KNOWBUG_SERVER_HSP3_ROOT:
    - [HSP のフルセット・アーカイブ版](http://hsp.tv/make/downlist.html)を新たに展開して、そのディレクトリへの絶対パスを指定する。
    - サーバー (knowbug_dll) をデバッグするために使う。
- KNOWBUG_CLIENT_HSP3_ROOT:
    - HSP のフルセット・アーカイブ版を新たに展開して、そのディレクトリへの絶対パスを指定する。
    - クライアント (knowbug_client) をデバッグするために使う。
    - KNOWBUG_SERVER_HSP3_ROOT と同じにはできない。

また、`MSBuild.exe` へのパスを通してください。(環境変数 Path に `MSBuild.exe` があるディレクトリへの絶対パスを追加してください。)

- `MSBuild.exe` は、Visual Studio 2019 なら `"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\"` にあります。

## デバッグ版のインストール

ビルドで生成される DLL を指すシンボリックリンクをインストールしておくと便利です。

シンボリックリンクを手動で作成するのはめんどうなので、スクリプトを用意しています。PowerShell (pwsh) を開き、以下のスクリプトを実行してください。

```pwsh
./scripts/install-dev.ps1
```

インストール先の環境でスクリプトをデバッグ実行するには `./scripts/run.ps1` が使用できます。`./scripts/run-default.ps1` は `./sandbox/default.hsp` を実行します。


```pwsh
./scripts/run-default.ps1
```

## デプロイ

- 作業を master ブランチにマージする。
- 変更履歴を更新する。(changes.md)
- バージョン番号を上げる。(変更箇所は前のバージョン番号で検索して見つける。)
- バージョン番号のタグを貼る。(例: `git tag v1.0.0`)
- ビルドとパッケージ作成のスクリプトを実行する。

```pwsh
./scripts/build-all
./scripts/pack
```

- パッケージを GitHub Releases にアップロードする。

## C++ コーディングメモ

コーディング上で迷ったときの参考用です。守らなくても可。

### C++ の資料

- [江添亮のC++入門](https://ezoeryou.github.io/cpp-intro/)
- [C++日本語リファレンス](https://cpprefjp.github.io/)

### 名前付け

- マクロ: `SCREAMING_CASE`
- 型、型引数、テンプレート引数: `PascalCase`
- メンバ変数: `snake_case_` (末尾に `_`)
- 名前空間、ローカル変数、関数: `snake_case`

### フォーマット

- `const` や `*` は後置 (例 `char const*`)
- 関数の結果型は後置 (例 `auto f() -> ResultType {..}`) (void だけ前置)
- Visual Studio の「ドキュメントのフォーマット」になるべく従う
    - ただし「else の前に改行する」設定だけ変えて「改行しない」ようにしている

## HSP コーディングメモ

### 名前付け

原則として `snake_case` です。(例外: Win32 API の定数など)

- 変数: `s_xxx`
    - ただし引数や local 変数には `s_` をつけません。
- ラベル: `l_xxx`
- モジュール名: `m_xxx`
- メンバ変数: `xxx_`
- 命令・関数: `xxx_yyy`
    - 例えばモジュール `m_foo` の中で定義される命令や関数は `foo_hello` のような名前にします。

## コミットメッセージ

Git のコミットメッセージについている "feat:" などのプレフィックスは、そのコミットの目的を表しています。付けなくても可。

- `feat`: 機能の変化 (追加・変更・削除) のためのコミット
- `fix`: 過去のコミットの誤りを修正するコミット
- `refactor`: コードの品質のためのコミット
    - リファクタリング、テストコードの追加など
    - 結果的に機能が変化しても可
- `docs`: ドキュメントの変更のコミット
- `chore`: 開発者のためのコミット (開発環境の設定の変更など)
