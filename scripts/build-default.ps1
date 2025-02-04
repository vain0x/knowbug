#!/bin/pwsh
# 既定のビルドを行う。
# 使い方:
#   ./scripts/build-default

# MSBuild.exe へのパスを取得する
. './scripts/_msbuild.ps1'
$MSBuild = $(getMSBuild)

& $MSBuild './src/knowbug.sln' "-p:Configuration=DebugUtf8;Platform=x86"
if (!$?) {
    write-error 'ビルドに失敗しました。'
    exit 1
}

& './src/knowbug_tests/bin/DebugUtf8/Win32/knowbug_tests.exe'
if (!$?) {
    write-error 'テストに失敗しました。'
    exit 1
}
