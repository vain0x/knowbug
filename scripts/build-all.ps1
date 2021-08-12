#!/bin/pwsh
# knowbug をすべての設定でビルドする。

# 使い方:
#   ./scripts/build-all

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

function build($config, $platform) {
    MSBuild.exe './src/knowbug.sln' "-p:Configuration=$config;Platform=$platform"
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

build 'Debug' 'x86'
build 'DebugUtf8' 'x86'
build 'DebugUtf8' 'x64'
build 'Release' 'x86'
build 'ReleaseUtf8' 'x86'
build 'ReleaseUtf8' 'x64'
