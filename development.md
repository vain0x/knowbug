# knowbug 開発者用メモ

## 開発環境

- Windows 10
- Visual Studio 2019
    - C++ 開発用の機能をインストールしておく。

## 環境変数

`scripts` にあるスクリプトは以下の環境変数を要求する。

- KNOWBUG_MSBUILD:
    - `MSBuild.exe` へのパスを指定しておく
    - `appveyor.yml`.environment.KNOWBUG_MSBUILD を参照

## ビルド

ソリューション (knowbug.sln) を Visual Studio で開いて「ビルド」(Ctrl+B)する。

- ビルドプロファイル
    - Debug/Release2
        - Debug は knowbug 自体のデバッグ用。Release2 は配布用。
        - どちらも hsp3 (shift_jis) ランタイム対応
    - DebugUtf/ReleseUtf
        - hsp3utf ランタイム対応 (hsp3debug_u8.dll)
    - Win32/x64
        - **Win32**: 32 bit 版 HSP 用
        - **x64**: 64 bit 版 HSP 用。hsp3debug_64.dll

## テスト

./package/sample/ のサンプルコードなどでちまちま動作確認する。

## デバッグ

ビルドで生成される DLL を指すシンボリックリンクを HSP のディレクトリに配置しておくと便利である。(FIXME: 作業手順を書く。 [install-dev](./scripts/install-dev.ps1) 参照)

HSP の起動時、あるいは起動中に Visual Studio の「プロセスにアタッチ」(Ctrl+Alt+P)をすると、ブレークポイントや assert で停止したときに knowbug 側のコードをトレース実行できる。

デバッグ用の knowbug の起動時にシフトキーを押しておくと、knowbug が初期化される前に停止するようになっているので、アタッチしやすい。

## デプロイ

- 作業を develop ブランチにマージする。
- バージョン番号を上げる。
- Visual Studio で Release 版 (x86/x64) をビルドする。
- 動作確認する。
- develop を master ブランチにマージしてタグを貼る。
- `make_deploy_package.bat` を実行する。
    - 新しいディレクトリー (knowbug) を作ってパッケージに同梱すべきファイル (DLL やサンプルコードなど) を放り込み、zip する。
    - 7zip のコマンドライン版が必要 (?)
    - 手動でも可。
- GitHub releases にアップロードする。

## コーディング規約

名前付けやフォーマットに迷ったときの参考用。ほとんどの部分で守られていないので、守らなくても可。

### 名前

- マクロ: `SCREAMING_CASE`
- 型、型引数、テンプレート引数: `PascalCase`
- メンバ変数: `snake_case_` (末尾に `_`)
- 名前空間、ローカル変数、関数: `snake_case`

### フォーマット

- `const` や `*` は後置 (例 `char const*`)
- 関数の結果型は後置 (例 `auto f() -> ResultType {..}`)

## コミットプレフィックス

Git のコミットメッセージについている "feat:" などは、そのコミットの目的を表す。付けなくても可。

- `feat`: 機能の変化 (追加・変更・削除) のためのコミット
- `fix`: 過去のコミットの誤りを修正するコミット
- `refactor`: コードの品質のためのコミット
    - リファクタリング、テストコードの追加など
    - 結果的に機能が変化しても可
- `docs`: ドキュメントの変更のコミット
- `chore`: 開発者のためのコミット (開発環境の設定の変更など)
