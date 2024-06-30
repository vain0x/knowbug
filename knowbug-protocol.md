# knowbug protocol

サーバーとクライアントの間の通信プロトコルのメモ。(仕様は未確定。)

## 用語

- デバッギー
    - スクリプトをデバッグ実行している HSP ランタイムのプロセス (hsp3.exe)
- サーバー
    - HSP のランタイムにロードされているデバッガーモジュール (hsp3debug.dll)
- クライアント
    - デバッグウィンドウを表示しているプロセス (knowbug_client.exe)

## 凡例

サーバーとクライアントは互いに以下の形式のメッセージを送ることで通信する。(mod_transfer_protocol.hsp を参照)

例:

```
method = initialized_event
version = v1.0.0
```

- メッセージは conf のような形式を使う。
    - 改行区切り
    - 各行は「キー = 値」の形式
    - (将来的には JSON にしたい。)
- 先頭の行は method でなければいけない。
- サーバーからクライアントに送るメソッドは xxx_event という名前になっている。
    - デバッギーが起こしたイベント (assert など) にデバッガーが反応するイメージ
- クライアントからサーバーに送るメソッドは xxx_notification という名前になっている。
    - ユーザーの操作をデバッガーモジュールやランタイムに通知するイメージ

## 起動

サーバーがクライアントを起動した後、クライアントは初期化が完了した旨をサーバーに通知する。

```
method = initialize_notification
client_hwnd = <hwnd>
```

ほかのすべてのメッセージについて、クライアントから initialize_notification を受け取った後にのみ、サーバーはクライアントにメッセージを送ってよい。

これを受信したサーバーは以下のメッセージを送る。サーバーのバージョン番号を含めてよい。

```
method = initialized_event
version = v1.0.0
```

## 終了

サーバーはクライアントにデバッグの終了を通知できる。

```
method = terminated_event
```

任意のタイミングで、クライアントはサーバーにデバッグの終了を通知できる。

```
method = terminate_notification
```

## ステップ実行

クライアントは以下のメッセージを送って、サーバーにデバッギーのステップ実行を要求できる。

実行の再開:

```
method = continue_notification
```

実行の中断:

```
method = pause_notification
```

ステップイン (「次へ」):

```
method = step_in_notification
```

ステップオーバー (「次飛」):

```
method = step_over_notification
```

ステップアウト (「脱出」):

```
method = step_out_notification
```

サーバーは以下のメッセージを送って、デバッギーの実行状態をクライアントに知らせることができる。(クライアントからのメッセージとは無関係に送ってもよい。)

実行の再開:

```
method = continued_event
```

実行の中断:

```
method = stopped_event
```

## 実行位置とソース

クライアントはサーバーにデバッギーの実行中の位置を要求できる。

```
method = location_notification
```

このメッセージを受信したサーバーは、可能ならクライアントに実行位置を知らせる。

```
method = location_event
source_file_id = <実行位置のソースファイルID>
line_index = <実行位置の行番号(0から始まる)>
```

ソースファイルIDは、ソースファイルを区別するために割り振られた1以上の整数。クライアントは以下のメッセージでソースファイルの詳細をサーバーに要求できる。

```
method = source_notification
source_file_id = <ソースファイルID>
```

サーバーは可能なら以下の応答を返す。

```
method = source_event
source_file_id = <ソースファイルID>
source_path = <ソースファイルのパス(UTF-8)>
source_code = <ソースコード(UTF-8)>
```

## オブジェクトリスト

クライアントはサーバーにオブジェクト (変数など) のデータを要求できる。

```
method = list_update_notification
```

サーバーはオブジェクトデータの変化に関して、以下の応答を好きな数だけ送る。

```
method = list_updated_event
kind = <insert または remove または update>
object_id = <オブジェクトID>
index = <リスト上のインデックス>
name = <オブジェクトの名前>
value = <オブジェクトの値>
count = <除去件数 (remove のときのみ)>
```

クライアントはサーバーにオブジェクトリストの詳細さの変更を要求できる。

```
method = list_toggle_expand_notification
object_id = <オブジェクトID>
```

サーバーは可能なら list_updated_event イベントで応答する。

クライアントはサーバーにオブジェクトの詳細情報を要求できる。

```
method = list_details_notification
object_id = <オブジェクトID>
```

サーバーは可能なら以下の応答を返す。

```
method = list_details_event
object_id = <オブジェクトID>
text = <テキスト(UTF-8)>
```

## 個別データのリクエスト

### 式の評価

リクエスト:

```
method = evaluation_notification
req_id = <数値>
expr = <式(UTF-8)>
```

レスポンス:

```
method = evaluated_event
req_id = <数値>
success = <成功したらtrue、失敗したらfalse>
value = <成功なら値を表現するテキスト>
type = <成功なら型を表現するテキストまたは空文字列>
```

### モジュール・静的変数

```
method = objects_update_notification
object_id = <オブジェクトID>
object_type = <オブジェクトの種類>
```

サーバーは可能なら以下の応答を返す。

```
method = objects_updated_event
data_count = <データ件数>

module_count = <モジュールの個数>
static_variable_count = <静的変数の個数>
```

加えて、サーバーはモジュールや静的変数のデータを好きな数だけ送る。

```
method = object_details_updated_event
object_id = <オブジェクトID>
object_type = <オブジェクト種類>
name = <モジュール名(UTF-8)>
value = <テキスト(UTF-8)>
```

## ログ

サーバーはデバッギーやサーバー自身が生成したログをクライアントに送信できる。

```
method = output_event
output = <テキスト(UTF-8)>
```
