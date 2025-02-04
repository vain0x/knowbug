#!/bin/pwsh
# knowbug をすべての設定でビルドする。

# 使い方:
#   ./scripts/build-all

# MSBuild.exe へのパスを取得する
. './scripts/_msbuild.ps1'
$MSBuild = $(getMSBuild)

function build($config, $platform) {
    & $MSBuild './src/knowbug.sln' "-p:Configuration=$config;Platform=$platform"
    if (!$?) {
        write-error "ビルドに失敗しました。($platform-$config)"
        exit 1
    }
}

./scripts/build-client
if (!$?) {
    write-error 'クライアントのビルドに失敗しました。'
    exit 1
}

./scripts/set-client-icon
if (!$?) {
    exit 1
}

build 'Debug' 'x86'
build 'DebugUtf8' 'x86'
build 'DebugUtf8' 'x64'
build 'Release' 'x86'
build 'ReleaseUtf8' 'x86'
build 'ReleaseUtf8' 'x64'
