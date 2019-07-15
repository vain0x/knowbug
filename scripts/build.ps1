# knowbug をビルドする。

# 使い方:
#   ./scripts/build [OPTIONS]
#
# 引数:
#   MSBuild のオプション

# MSBuild を探す。
# Visual Studio 2019
$msBuild="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

$knowbugRoot = (get-item .).FullName

try {
    cd "$knowbugRoot/src"

    & $msBuild knowbug.vcxproj $args
    if (!$?) {
        exit 1
    }
} finally {
    cd $knowbugRoot
}
