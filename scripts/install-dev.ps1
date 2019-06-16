# knowbug のデバッグ版をインストールする。

# 条件:
#   環境変数 HSP3_ROOT に HSP のディレクトリを設定しておく。

# 使い方:
#   (管理者権限で実行する。)
#   echo $env:HSP3_ROOT
#   cd knowbug
#   ./scripts/install-dev.ps1

$H = $env:HSP3_ROOT
$K = (get-item .).FullName

# HSP のデバッガを改名する。
move-item -force "$H/hsp3debug.dll" "$H/hsp3debug__default.dll"
move-item -force "$H/hsp3debug_u8.dll" "$H/hsp3debug__u8_default.dll"
move-item -force "$H/hsp3debug_64.dll" "$H/hsp3debug__64_default.dll"

# knowbug のデバッグビルドへのシンボリックリンクを張る。
new-item -itemType symbolicLink -path "$H/hsp3debug.dll" -value "$K/src/Debug/hsp3debug.dll"
new-item -itemType symbolicLink -path "$H/hsp3debug_u8.dll" -value "$K/src/Debug/u8/hsp3debug_u8.dll"
new-item -itemType symbolicLink -path "$H/hsp3debug_64.dll" -value "$K/src/x64/Debug/hsp3debug_64.dll"
