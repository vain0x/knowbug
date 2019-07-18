# パッケージを生成する。
# 使い方: ./scripts/pack

# バージョン番号
$version = $env:APPVEYOR_BUILD_VERSION
if (!$version) {
    $version = "next"
}

# パッケージのファイル名
$package = "knowbug-$version"

# パッケージから除外されるファイル (package/ 以下)
$exclusions = @(
    "hsptmp",
    "obj",
    "install"
)

# パッケージに含められるファイル (package/ 以外)
$inclusions = @(
    @("$package/hsp3debug.dll", "$pwd/src/Release/hsp3debug.dll"),
    @("$package/hsp3debug_u8.dll", "$pwd/src/Release/u8/hsp3debug_u8.dll"),
    @("$package/hsp3debug_64.dll", "$pwd/src/x64/Release2/hsp3debug_64.dll")
)

if (test-path "$package.zip") {
    remove-item "$package.zip"
}

mkdir $package
try {
    copy -recurse package $package
    copy changes.md $package/changes.md
    copy README.md $package/README.md
    copy LICENSE $package/LICENSE

    foreach ($name in $exclusions) {
        remove-item -recurse -force "$package/**/$name"
    }

    foreach ($row in $inclusions) {
        $targetPath = $row[0]
        $sourcePath = $row[1]
        copy $sourcePath $targetPath
    }

    compress-archive "$package/*" "$package.zip"

    echo "HINT: 'expand-archive $package.zip' to expand"
    echo "Completed."
} finally {
    rm -recurse -force $package
}
