# すべてのテストを実行する。

# Visual Studio 2019
$msBuild="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"

$knowbugRoot = (get-item .).FullName

$table = @(
    @("/p:Configuration=Debug;Platform=x86", "knowbug_tests/Debug/knowbug_tests.exe"),
    @("/p:Configuration=DebugUtf8;Platform=x86", "knowbug_tests/DebugUtf8/knowbug_tests.exe"),
    @("/p:Configuration=Debug;Platform=x64", "x64/Debug/knowbug_tests.exe")
)

$success = $true

try {
    cd "$knowbugRoot/src"

    foreach ($row in $table) {
        $config = $row[0]
        $exe = $row[1]

        & $msBuild knowbug_tests /t:build $config
        if (!$?) {
            $success = $false
            continue
        }

        & $exe
        if (!$?) {
            $success = $false
            continue
        }
    }
} finally {
    cd $knowbugRoot
}

if (!$success) {
    exit 1
}
