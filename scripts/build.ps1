# knowbug をビルドする。

# 使い方:
#   ./scripts/build [OPTIONS]
#
# 引数:
#   MSBuild のオプション

# Visual Studio 2019
$MSBuild="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

$knowbugRoot = (get-item .).FullName

try {
    cd "$knowbugRoot/src"

    & $MSBuild knowbug.vcxproj $args
} finally {
    cd $knowbugRoot
}
