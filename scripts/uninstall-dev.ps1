# デバッガーをバックアップから復元する。

if (!$env:KNOWBUG_SERVER_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_SERVER_HSP3_ROOT を設定してください。'
    exit 1
}

function uninstall($name) {
    $root = (get-item $env:KNOWBUG_SERVER_HSP3_ROOT).fullName
    $src = "$root/.backup/$name"
    $dest = "$root/$name"

    if ($(test-path $dest)) {
        remove-item $dest
    }

    copy-item -path $src -destination $dest
}

uninstall 'hsp3debug.dll'
uninstall 'hsp3debug_u8.dll'
uninstall 'hsp3debug_64.dll'
