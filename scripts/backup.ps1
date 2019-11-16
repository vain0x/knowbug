# デバッガーのバックアップを作成する。
# すでにバックアップがあるときは何もしない。

if (!$env:KNOWBUG_SERVER_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_SERVER_HSP3_ROOT を設定してください。'
    exit 1
}

function backup($name) {
    $root = (get-item $env:KNOWBUG_SERVER_HSP3_ROOT).fullName

    $src = "$root/$name"
    $dest = "$root/.backup/$name"

    mkdir -force "$root/.backup"

    if (!$(test-path $src)) {
        echo "ファイルが見つかりません。($name)"
        return
    }

    if ($(test-path $dest)) {
        echo "バックアップはすでに存在します。($name)"
        return
    }

    echo "バックアップを作成します。($name)"
    copy-item -path $src -destination $dest
}

backup 'hsp3debug.dll'
backup 'hsp3debug_u8.dll'
backup 'hsp3debug_64.dll'
