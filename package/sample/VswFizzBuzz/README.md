# VswFizzBuzz
`int` 型の値を FizzBuzz 形式で出力するプラグイン

* 簡単な vsw プラグインのサンプル

## 導入
0. VswFizzBuzz をビルドして、VswFizzBuzz.dll をHSPのフォルダに置く。
0. 設定ファイル `knowbug.ini` に以下のように追記する。
  * ini ファイルの仕様に注意すべし。

```ini
[VardataString/UserdefTypes]
int="fizzbuzz_vsw.dll"

[VardataString/UserdefTypes/Func]
int.receiveVswMethods="_receiveVswMethods@4"
int.addValue="_addValueInt@12"
```

## 実行例
```hsp
  repeat 16
    ι(cnt) = cnt
  loop
```

![実行例](https://pbs.twimg.com/media/CT3DkXwUsAQouwR.png)
