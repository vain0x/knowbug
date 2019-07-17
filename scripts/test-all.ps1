#!/bin/pwsh
# すべてのテストを実行する。

$msBuild = $env:KNOWBUG_MSBUILD
if (!$msBuild) {
    write-error "環境変数 KNOWBUG_MSBUILD を設定してください"
    exit 1
}

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

        # -t:build,run を指定するとビルド後に実行されるが、なぜか文字化けするので使わない。
        & $msBuild knowbug_tests -t:build $config
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
