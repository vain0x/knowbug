# knowbug をすべての設定でビルドする。

# 使い方:
#   ./scripts/build

$configs = @(
    "/p:Configuration=Debug;Platform=x86",
    "/p:Configuration=DebugUtf8;Platform=x86",
    "/p:Configuration=Debug;Platform=x64",
    "/p:Configuration=Release2;Platform=x86",
    "/p:Configuration=ReleaseUtf8;Platform=x86",
    "/p:Configuration=Release2;Platform=x64",
)

foreach ($config in $configs) {
    ./build /t:build $config
}
