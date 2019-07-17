# knowbug をビルドする。

# 使い方:
#   ./scripts/build [OPTIONS]
#
# 引数:
#   MSBuild のオプション

# Visual Studio 2019
$msBuild="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

$knowbugRoot = (get-item .).FullName

try {
    cd "$knowbugRoot/src"

    & $msBuild knowbug.vcxproj $args
    if (!$?) {
        exit 1
    }

    # テストを実行する。 (-t:Build,Run を使うと実行できるが、なぜか文字化けする。)
    & $msBuild knowbug_tests "-t:Build" "-p:Configuration=DebugUtf8;Platform=x86"
    if (!$?) {
        exit 1
    }

    & "knowbug_tests/DebugUtf8/knowbug_tests"
    if (!$?) {
        exit 1
    }
} finally {
    cd $knowbugRoot
}
