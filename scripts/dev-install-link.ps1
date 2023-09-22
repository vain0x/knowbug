#!/bin/pwsh
# knowbug 開発用の HSP3 に knowbug をインストールする。
#
# (knowbug をデバッグビルドしたときに生成される DLL を指すシンボリックリンクを作る。
#  シンボリックリンクは、それが指しているファイルの代わりになる。
#  これにより、開発環境で常に最新のデバッグビルドを使えるようになる。)

$ErrorActionPreference = 'Stop'

$serverDir = "$PWD/bin/server"

if (!(test-path $serverDir)) {
    write-error 'ERROR: bin/server がありません。'
    exit 1
}

function install($name, $config, $platform) {
    $dest = "$serverDir/$name"
    $src = "$PWD/src/knowbug_dll/$config/$platform/$name"

    # 対象のファイルがなければ作成する。(存在しないファイルへのリンクは作成できないため。)
    mkdir -force ([System.IO.Path]::GetDirectoryName($src))
    if (!(test-path $src)) {
        new-item $src
    }

    # ファイルパスを絶対パスにする。
    $src = (get-item $src).fullName

    # 古いリンクがあれば削除する。
    if (test-path $dest) {
        remove-item -force $dest
    }

    echo 'dest = ' $dest
    echo 'src = ' $src
    new-item -itemType symbolicLink -path $dest -value $src
}

install 'hsp3debug.dll' 'Debug' 'Win32'
install 'hsp3debug_u8.dll' 'DebugUtf8' 'Win32'
install 'hsp3debug_64.dll' 'DebugUtf8' 'x64'

echo 'INFO: knowbug のデバッグビルドを用意しました。'
