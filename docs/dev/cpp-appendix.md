# C++ の補足資料

C++ の機能のうち、このリポジトリで使っているものを取り上げて説明する資料です。
「C++ 入門」と「このリポジトリで使っている C++」の間を埋めることを目的としています

C++ の基本的な使いかたについては入門用の書籍やウェブサイトを参照してください



## 変数宣言: `auto … = …`

C++の変数宣言は基本的に `型 変数名 = 初期値` と書きますが、`auto` キーワードを使うことで変数の型を書かずに済ませることができます

```cpp
    // 変数 s の型は、getString の戻り値の型に等しい
    auto s = getString();
```

このリポジトリでは、ローカル変数の宣言は基本的にこの構文を使っています。
理由は2つあり、型の名前が長いときに見やすいことと、意図しない暗黙変換を防ぐためです
(後者の詳細はリンク先を参照)

- 参考: [auto (cpprefjp)](https://cpprefjp.github.io/lang/cpp11/auto.html)

#### `auto&&` 系の使い分け

`auto` の変種として `auto const&` や `auto&&` などを使っていることがあります。
`&` つきのものを使うと変数は参照型になります。
(単に `auto` だけだと参照型にならない)

主に4種類あります:

- `auto`
    - たいていこれでOK
- `auto const&`: (読み取り専用の参照)
    - `auto` とほとんど同様で、基本的に `auto` でいい、ただし `auto const&` を使うべき場合もある
    - 一般論は後述
- `auto&&`: ([ユニバーサル参照])
    - [範囲for文] (`for (auto&& item : iter)` という構文) で使う
- `auto&`: (読み書き可能な参照)
    - 書き込み用のオブジェクトへの参照を関数から受け取るときに使う
        (例 `auto& w = writer();`)
        - `writer().do_something()` の代わりに `w.do_something()` と書けて楽になる

`auto` と `auto const&` の使い分けはややこしいです。
基本的に `auto` でいいです

#### `auto const&` の詳細

(※過剰にややこしいので簡略化したい)

**コピーのコストが低い型** のオブジェクトを変数で持つ場合、後述の条件とは関係なく、`auto` を使ったほうがよいです。
コピーのコストが低いのは主に整数型、ポインタ、参照、`string_view` 系の型です。
`auto const&` を使うのは不要なコピーを避けることで性能悪化を防ぐためです。
これらの型はもとからコピーのコストが実質ゼロなので、`auto const&` を使う意味がありません

`auto const&` を使うのは主に次の2パターンです:

- `T const&` を返す関数の返り値を変数で持つとき
- ローカル変数の一部 (メンバや配列要素) を別の変数に割り当てるとき (例 `auto const& item = array[i]; f(item);`)

`auto const&` は参照なので、その変数が生存している間ずっと **参照先のオブジェクトが生存していること** という条件が必要です。
オブジェクトを所有している場合と、すでにほかの変数が同じオブジェクトへの参照を持っている場合 (かつその変数が同条件を満たす場合) に、この条件を満たします。
(ここでオブジェクトを **所有** しているというのは、関数内のローカル変数やメンバ変数が参照ではない型でオブジェクトを持っているということ)



## 関数宣言の構文: `auto … -> R`

```cpp
    int f();
    // ↑↓ 同じ意味です
    auto f() -> int;
```

C++の関数宣言は `型 関数名(パラメータ…) …` と書きますが、`auto` キーワードを使って次のように書くこともできます:

```cpp
    auto 関数名(パラメータ…) -> 型 …
```

このリポジトリでは `auto` から始まるこの構文を好んで使っています。
理由は戻り値型が長くなったときに見やすいからです。
好みの問題ですが、従来の構文は関数名が戻り値型とパラメータリストに挟まれて見づらいと思います

```cpp
    SomeLongNameReturnType F1(Parameter1, Parameter2)…
    AnotherLongNameReturnType F2(Parameter3, Parameter4)…
    // ↑↓ 同じ
    auto F1(Parameter1, Parameter2) -> SomeLongNameReturnType…
    auto F2(Parameter3, Parameter4) -> AnotherLongNameReturnType…
```

戻り値型が `void` のときは従来の構文を使って `void 関数名()…` と書いている場合もあります

- 参考: [戻り値の型を後置する関数宣言構文 (cpprefjp)](https://cpprefjp.github.io/lang/cpp11/trailing_return_types.html)

#### 戻り値型の推論

ちなみに `-> 型` の部分を書かない場合、関数本体から推論する機能があります。
このリポジトリではこの機能を使っていないはずです

- 参考: [通常関数の戻り値型推論 (cpprefjp)](https://cpprefjp.github.io/lang/cpp14/return_type_deduction_for_normal_functions.html)



## 一様初期化: `T{…}`

`T` を型の名前とすると、`T{…}` という式でその型の値を作れます

size_t型の `0` を表すために `size_t{}` と書いている場所があります。
同様に特定のポインタ型のNULLポインタを `HWND{}` などと書いている場所があります
(※これはいま思うと過剰だった)

```cpp
    size_t i = 0;
    // ↑↓ 同じ
    auto i = size_t{};
```

- 参考: [一様初期化 (cpprefjp)](https://cpprefjp.github.io/lang/cpp11/uniform_initialization.html)



----

## 資料

- [C++ 書籍 - C++ の歩き方 | cppmap](https://cppmap.github.io/learn/books/)
- [C++日本語リファレンス](https://cpprefjp.github.io/)

その他

- 公式のドキュメント: [Visual Studio の C および C++ | Microsoft Learn](https://learn.microsoft.com/ja-jp/cpp/overview/visual-cpp-in-visual-studio?view=msvc-170)



[ユニバーサル参照]: https://cpprefjp.github.io/lang/cpp11/rvalue_ref_and_move_semantics.html
[範囲for文]: https://cpprefjp.github.io/lang/cpp11/range_based_for.html
