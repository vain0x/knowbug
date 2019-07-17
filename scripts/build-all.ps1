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
    write-error "環境変数 KNOWBUG_MSBUILD を設定してください"
    exit 1
}

$knowbugRoot = (get-item .).FullName

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
