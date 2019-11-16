# hsp3_make.hsp をコンパイルする。
# HSP スクリプトの実行ファイルを作成するためのスクリプト。

if (!$env:KNOWBUG_CLIENT_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_CLIENT_HSP3_ROOT を設定してください。'
    exit 1
}

$workDir = (get-item .).fullName
try {
    cd './scripts'

    $makeScript = (get-item 'hsp3_make.hsp').fullName

    # 実行ファイル生成用のスクリプトをコンパイルする。
    & "$env:KNOWBUG_CLIENT_HSP3_ROOT/hspcmp.exe" "--compath=$env:KNOWBUG_CLIENT_HSP3_ROOT\\common\\" $makeScript
    if (!$?) {
        write-error 'hsp3_make.hsp のコンパイルに失敗しました。'
        exit 1
    }
} finally {
    cd $workDir
}
