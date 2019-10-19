# パッケージを生成する。
# 使い方: ./scripts/pack

# バージョン番号
$version = $env:APPVEYOR_BUILD_VERSION
if (!$version) {
    $version = "next"
}

# パッケージのファイル名
$package = "knowbug-$version"

# パッケージから除外されるファイル (dist/ 以下)
$exclusions = @(
    "hsptmp",
    "obj"
)

# パッケージに含められるファイル (dist/ 以外)
$inclusions = @(
    @("$package/changes.md", "$pwd/changes.md"),
    @("$package/LICENSE", "$pwd/LICENSE"),
    @("$package/README.md", "$pwd/README.md"),
    @("$package/knowbug_install.exe", "$pwd/src/knowbug_install/knowbug_install.exe"),
    @("$package/hsp3debug.dll", "$pwd/src/Win32/Release/hsp3debug.dll"),
    @("$package/hsp3debug_u8.dll", "$pwd/src/Win32/ReleaseUtf8/hsp3debug_u8.dll"),
    @("$package/hsp3debug_64.dll", "$pwd/src/x64/Release/hsp3debug_64.dll")
)

if (test-path "$package.zip") {
    remove-item "$package.zip"
}

mkdir $package
try {
    # dist の中にある各ファイルをパッケージに含める。(dist/ ディレクトリ自体は含めない。)
    copy -recurse dist/* $package

    # 配布しないファイルを削除する。(ファイル名が一致するのものをすべて削除する。)
    foreach ($name in $exclusions) {
        remove-item -recurse -force "$package/**/$name"
    }

    # dist の外にあるファイルをパッケージに含めるためにコピーする。
    foreach ($row in $inclusions) {
        $targetPath = $row[0]
        $sourcePath = $row[1]
        copy $sourcePath $targetPath
    }

    # パッケージを圧縮する。
    compress-archive $package "$package.zip"

    echo "HINT: 'expand-archive $package.zip' to expand"
    echo "Completed."
} finally {
    rm -recurse -force $package
}
