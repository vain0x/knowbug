# knowbug 開発者用メモ

## 開発環境

- Windows 10
- Visual Studio 2015
    - C++ 開発用の機能をインストールしておく。

## ビルド

ソリューション (knowbug.sln) を Visual Studio で開いて「ビルド」(Ctrl+B)する。

- ビルドプロファイル
    - Debug/Release
        - Debug は knowbug 自体のデバッグ用。Release は配布用。
    - x86/x64
        - **x86**: 32 bit 版 HSP 用
        - **x64**: 64 bit 版 HSP 用。hsp3debug_64.dll

## テスト

./package/sample/ のサンプルコードなどでちまちま検証する。

## デバッグ

Debug モードでビルドしたものをHSPの hsp3debug.dll と入れ替えて、HSPを実行する。Visual Studio の「プロセスにアタッチ」(Ctrl+Alt+P)でHSPのランタイム (hsp3.exe) にアタッチすると、ブレークポイントに止まって、変数の値を確認したりトレース実行したりできる。

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
