# knowbug のデバッグ版をインストールする。

# (どういうわけかこれを実行しても動作しない。参考程度に使う。)

# 条件:
#   環境変数 HSP3_ROOT に HSP のディレクトリを設定しておく。

# 使い方:
#   (管理者権限で実行する。)
#   echo $env:HSP3_ROOT
#   cd knowbug
#   ./scripts/install-dev.ps1

if (!$env:HSP3_ROOT) {
    write-error "環境変数 HSP3_ROOT を設定してください"
    exit 1
}

$table = @(
    @("hsp3debug.dll", "$pwd/src/knowbug_dll/bin/Debug/Win32/hsp3debug.dll"),
    @("hsp3debug_u8.dll", "$pwd/src/knowbug_dll/bin/DebugUtf8/Win32/hsp3debug_u8.dll"),
    @("hsp3debug_64.dll", "$pwd/src/knowbug_dll/bin/DebugUtf8/x64/hsp3debug_64.dll")
)

foreach ($row in $table) {
    $name = $row[0]
    $targetPath = $row[1]
    $sourcePath = "$env:HSP3_ROOT/$name"
    $backup = "$env:HSP3_ROOT/$name.orig"

    # ファイルを移動する。
    echo "move $sourcePath -> $backup"
    move-item -force -path $sourcePath -destination $backup

    # シンボリックリンクを張る。
    echo "link $sourcePath -> $backup"
    new-item -itemType symbolicLink -path $sourcePath -value $targetPath
}
