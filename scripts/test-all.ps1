#!/bin/pwsh
# すべてのテストを実行する。

# MSBuild.exe へのパスを取得する
. './scripts/_msbuild.ps1'
$MSBuild = $(getMSBuild)

$knowbugRoot = (get-item .).FullName

function test($config, $platform) {
    # -t:build,run を指定するとビルド後に実行されるが、なぜか文字化けするので使わない。
    & $MSBuild './src/knowbug.sln' -t:build "-p:Configuration=$config;Platform=$platform"
    if (!$?) {
        write-error 'ビルドに失敗しました。'
        exit 1
    }

    if ($platform -eq 'x86') {
        $platform = 'Win32'
    }

    & "./src/knowbug_tests/bin/$config/$platform/knowbug_tests.exe"
    if (!$?) {
        write-error 'テストに失敗しました。'
        exit 1
    }
}

test 'Debug' 'x86'
test 'DebugUtf8' 'x86'
test 'DebugUtf8' 'x64'
