#!/bin/pwsh
# 既定のビルドを行う。
# 使い方:
#   ./scripts/build-default

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

$knowbugRoot = (get-item .).FullName

try {
    cd "$knowbugRoot/src"

    MSBuild.exe knowbug.sln "-p:Configuration=DebugUtf8;Platform=x86"
    if (!$?) {
        exit 1
    }

    & "Win32/DebugUtf8/knowbug_tests"
    if (!$?) {
        exit 1
    }
} finally {
    cd $knowbugRoot
}
