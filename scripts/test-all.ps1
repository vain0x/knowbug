#!/bin/pwsh
# すべてのテストを実行する。

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

$knowbugRoot = (get-item .).FullName

function test($config, $platform) {
    # -t:build,run を指定するとビルド後に実行されるが、なぜか文字化けするので使わない。
    MSBuild.exe './src/knowbug.sln' -t:build "-p:Configuration=$config;Platform=$platform"
    if (!$?) {
        write-error 'ビルドに失敗しました。'
        exit 1
    }

    if ($platform -eq 'x86') {
        $platform = 'Win32'
    }

    & "./src/target/knowbug_tests-$platform-$config/bin/knowbug_tests.exe"
    if (!$?) {
        write-error 'テストに失敗しました。'
        exit 1
    }
}

test 'Debug' 'x86'
test 'DebugUtf8' 'x86'
test 'DebugUtf8' 'x64'
