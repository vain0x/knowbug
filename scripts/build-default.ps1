#!/bin/pwsh
# 既定のビルドを行う。
# 使い方:
#   ./scripts/build-default

$msBuild = $env:KNOWBUG_MSBUILD
if (!$msBuild) {
    # NOTE: 文字列中に非 ASCII 文字があると構文エラーになることがある
    write-error "Environmnet variable KNOWBUG_MSBUILD is missing"
    exit 1
}

$knowbugRoot = (get-item .).FullName

try {
    cd "$knowbugRoot/src"

    & $msBuild knowbug.sln "-p:Configuration=DebugUtf8;Platform=x86"
    if (!$?) {
        exit 1
    }

    & "DebugUtf8/knowbug_tests"
    if (!$?) {
        exit 1
    }
} finally {
    cd $knowbugRoot
}
