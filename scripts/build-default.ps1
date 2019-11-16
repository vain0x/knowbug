#!/bin/pwsh
# 既定のビルドを行う。
# 使い方:
#   ./scripts/build-default

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

MSBuild.exe './src/knowbug.sln' "-p:Configuration=DebugUtf8;Platform=x86"
if (!$?) {
    write-error 'ビルドに失敗しました。'
    exit 1
}

& './src/target/knowbug_tests-Win32-DebugUtf8/bin/knowbug_tests.exe'
if (!$?) {
    write-error 'テストに失敗しました。'
    exit 1
}
