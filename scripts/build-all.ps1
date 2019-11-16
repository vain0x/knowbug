#!/bin/pwsh
# knowbug をすべての設定でビルドする。

# 使い方:
#   ./scripts/build-all

if (!$(which MSBuild.exe)) {
    write-error 'MSBuild.exe にパスを通してください。'
    exit 1
}

$knowbugConfigs = @(
    "-p:Configuration=Debug;Platform=x86",
    "-p:Configuration=DebugUtf8;Platform=x86",
    "-p:Configuration=Debug;Platform=x64",
    "-p:Configuration=Release;Platform=x86",
    "-p:Configuration=ReleaseUtf8;Platform=x86",
    "-p:Configuration=Release;Platform=x64",
    "-p:Configuration=ReleaseUtf8;Platform=x64"
)

$knowbugRoot = (get-item .).FullName

$success = $true

try {
    cd "$knowbugRoot/src"

    foreach ($config in $knowbugConfigs) {
        MSBuild.exe knowbug.sln -t:Build $config
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
