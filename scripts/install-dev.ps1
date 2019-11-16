# knowbug のデバッグ版をインストールする。
# シンボリックリンクを作成するため、管理者権限が要求される。

if (!$env:KNOWBUG_SERVER_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_SERVER_HSP3_ROOT を設定してください。'
    exit 1
}

function escalate() {
    if (!$(test-path '.lock')) {
        new-item '.lock'
        start-process pwsh -argumentList @('/c', './scripts/install-dev.ps1; pause') -verb 'runas'
        exit 0
    }
    remove-item '.lock'
}

function install($name, $config, $platform) {
    $root = (get-item $env:KNOWBUG_SERVER_HSP3_ROOT).fullName
    $src = (get-item "./src/target/knowbug_dll-$platform-$config/bin/$name").fullName
    $dest = "$root/$name"

    if ($(test-path $dest)) {
        remove-item $dest
    }

    new-item -itemType symbolicLink -path $dest -value $src
}

./scripts/backup
./scripts/build-all

escalate
install 'hsp3debug.dll' 'Debug' 'Win32'
install 'hsp3debug_u8.dll' 'DebugUtf8' 'Win32'
install 'hsp3debug_64.dll' 'Debug' 'x64'
