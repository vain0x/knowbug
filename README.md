# knowbug

**knowbug** は非公式の HSP3 用デバッグ・ウィンドウです。

[最新版はこちら](https://github.com/vain0x/knowbug/releases/latest)

![スクリーンショット](./screenshots/demo.gif)

主な機能としては以下があります。**[機能の詳細はこちらを参照](details.md)** してください。

- 多次元配列やモジュール変数の表示
- ステップオーバーなどの拡張されたステップ実行

## インストール

配布物に含まれている [knowbug_install.exe](./src/knowbug_install/knowbug_install.hsp) を管理者権限で実行してください。HSP のディレクトリを選択し install ボタンを押すと、インストールが行われます。

### コールスタック表示機能のインストール

**この手順は省略可能です。**

コールスタック (ユーザー定義命令の呼び出し履歴) の表示を行うには、追加のプラグイン (WrapCall) を読み込む必要があります。プラグインを読み込むには、`WrapCall.as` を `#addition` してください。(これはデバッグ時にのみ動作します。)

```hsp
#addition "WrapCall.as"
```

ただし、UTF-8 版や 64 ビット版の HSP を使う場合は、`hsp3utf.as` や `hsp3_64.as` より **後に** WrapCall.as を #addition してください。

```hsp
#include "hsp3utf.as"  // こっちが前
#addition "WrapCall.as" // こっちが後
```

### 設定ファイルのインストール

**この手順は省略可能です。**

HSP のディレクトリに [knowbug.conf](./dist/knowbug.conf) を置いておくと、起動時に読み込まれます。具体的な設定は、設定ファイル内のコメントを参照してください。

## アンインストール

インストール時と同様に `knowbug_install.exe` で knowbug をインストールした HSP のディレクトリを指定し、 uninstall を選択してください。

- 設定ファイルを配置した場合は、手動で削除してください。

## 推奨環境

- HSP 3.6
    - 特に64ビット版は、HSP3.6から UTF-8 に変更されました。HSP3.51以前の環境では文字化けする可能性があります。
- Windows 10

## 不具合報告など

既知の不具合や要望などは以下にまとまっています。不具合の報告やご意見などは、ここに書いてもらえると嬉しいです。

- [Issues](https://github.com/vain0x/knowbug/issues)

knowbug のリリースの一覧は以下を参照してください。(pre-release は動作が安定していない可能性があります。)

- [Releases](https://github.com/vain0x/knowbug/releases)

## 開発者向けのドキュメント

- [ARCHITECTURE.md](ARCHITECTURE.md)
- [docs/dev](docs/dev)

## 関連リンク

- OpenHSP <http://dev.onionsoft.net/trac>
- HSP3 GINGER <https://github.com/vain0x/hsp3-ginger>
