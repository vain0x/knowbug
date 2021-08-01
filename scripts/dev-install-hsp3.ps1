#!/bin/pwsh
# knowbug の開発環境として HSP3 をインストールする。

$ErrorActionPreference = 'Stop'

# ------------------------------------------------
# 定数など
# ------------------------------------------------

# HSP のダウンロード元とバージョン。 ([HSP3.6RC2を公開しました - おにたま(オニオンソフト)のおぼえがき](https://www.onionsoft.net/wp/archives/3473)) (2021-08-01 閲覧)

$hsp3ZipUrl = 'https://www.onionsoft.net/hsp/file/hsp36rc2.zip'
$hsp3ZipDir = 'hsp36' # zipを展開して作られるディレクトリ

# 開発用の HSP をインストールする場所。
$clientDir = "$PWD/bin/client"
$serverDir = "$PWD/bin/server"

# ZIP ファイルを置く場所。(バージョン番号 + 適当な数値)
$n = ([System.DateTime]::Now).Ticks
$hsp3Zip = "$PWD/tmp/hsp3-$n.zip"

# ------------------------------------------------
# 検査
# ------------------------------------------------

if ((get-item $PWD).name -ne 'knowbug') {
    echo 'ERROR: knowbug ディレクトリで実行してください。'
    exit 1
}

# ------------------------------------------------
# インストール
# ------------------------------------------------

# 一時ディレクトリがなかったら作る。
mkdir -force tmp

function install_hsp3($name, $dest) {
    if (test-path "$dest/hsp3.exe") {
        echo "INFO: $name はインストール済みでした: '$dest'"
        return
    }

    # ZIP ファイルをダウンロードする。
    if (!(test-path $hsp3Zip)) {
        echo "INFO: HSP3 をダウンロードしています: '$hsp3ZipUrl'"
        curl -sL $hsp3ZipUrl -o $hsp3Zip
    }

    echo "INFO: $name をインストールしています: '$dest'"

    # インストール先のディレクトリの親がなければ生成し、
    # インストール先のディレクトリ自体が存在するなら削除しておく。
    mkdir -force $dest
    rm -recurse -force $dest

    # 圧縮ファイルを一時ディレクトリに展開する。(expand-archive だと文字コードの問題でエラーになる模様？)
    $shiftJis = [System.Text.Encoding]::GetEncoding(932)
    [System.IO.Compression.ZipFile]::ExtractToDirectory($hsp3Zip, "$PWD/tmp", $shiftJis)

    mv "$PWD/tmp/$hsp3ZipDir" $dest

    echo "INFO: $name をインストールしました。"
}

install_hsp3 'クライアント用の HSP3' $clientDir
install_hsp3 'サーバー用の HSP3' $serverDir

# ------------------------------------------------
# クリーンアップ
# ------------------------------------------------

# 一時ディレクトリを削除する。
rm -d -force tmp

# ZIP ファイルを削除する。
if (test-path $hsp3Zip) {
    rm -force $hsp3Zip
}

echo 'INFO: 開発用の HSP3 の用意ができました。'
