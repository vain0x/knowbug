#!/bin/pwsh
# すべてのテストを実行する。

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

$knowbugRoot = (get-item .).FullName

$table = @(
    @("/p:Configuration=Debug;Platform=x86", "Win32/Debug/knowbug_tests.exe"),
    @("/p:Configuration=DebugUtf8;Platform=x86", "Win32/DebugUtf8/knowbug_tests.exe"),
    @("/p:Configuration=Debug;Platform=x64", "x64/Debug/knowbug_tests.exe")
)

$success = $true

try {
    cd "$knowbugRoot/src"

    foreach ($row in $table) {
        $config = $row[0]
        $exe = $row[1]

        # -t:build,run を指定するとビルド後に実行されるが、なぜか文字化けするので使わない。
        MSBuild.exe knowbug.sln -t:build $config
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
