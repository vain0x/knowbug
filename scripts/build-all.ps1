#!/bin/pwsh
# knowbug をすべての設定でビルドする。

# 使い方:
#   ./scripts/build-all

$knowbugConfigs = @(
    "-p:Configuration=Debug;Platform=x86",
    "-p:Configuration=DebugUtf8;Platform=x86",
    "-p:Configuration=Debug;Platform=x64",
    "-p:Configuration=Release2;Platform=x86",
    "-p:Configuration=ReleaseUtf8;Platform=x86",
    "-p:Configuration=Release2;Platform=x64"
)

$msBuild = $env:KNOWBUG_MSBUILD
if (!$msBuild) {
    # NOTE: 文字列中に非 ASCII 文字があると構文エラーになることがある
    write-error "Environmnet variable KNOWBUG_MSBUILD is missing"
    exit 1
}

$knowbugRoot = (get-item .).FullName

$success = $true

try {
    cd "$knowbugRoot/src"

    foreach ($config in $knowbugConfigs) {
        & $msBuild knowbug.vcxproj -t:Build $config
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
