#!/bin/pwsh
# すべてのテストを実行する。

$msBuild = $env:KNOWBUG_MSBUILD
if (!$msBuild) {
    # NOTE: 文字列中に非 ASCII 文字があると構文エラーになることがある
    write-error "Environmnet variable KNOWBUG_MSBUILD is missing"
    exit 1
}

$knowbugRoot = (get-item .).FullName

$table = @(
    @("/p:Configuration=Debug;Platform=x86", "Debug/knowbug_tests.exe"),
    @("/p:Configuration=DebugUtf8;Platform=x86", "DebugUtf8/knowbug_tests.exe"),
    @("/p:Configuration=Debug;Platform=x64", "x64/Debug/knowbug_tests.exe")
)

$success = $true

try {
    cd "$knowbugRoot/src"

    foreach ($row in $table) {
        $config = $row[0]
        $exe = $row[1]

        # -t:build,run を指定するとビルド後に実行されるが、なぜか文字化けするので使わない。
        & $msBuild knowbug.sln -t:build $config
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
