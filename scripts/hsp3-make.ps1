if (!$env:HSP3_ROOT) {
    write-error '環境変数 HSP3_ROOT を設定してください。'
    exit 1
}

$workDir = (get-item .).fullName
try {
    cd './scripts'

    $makeScript = (get-item 'hsp3_make.hsp').fullName

    # 実行ファイル生成用のスクリプトをコンパイルする。
    & "$env:HSP3_ROOT/hspcmp.exe" "--compath=$env:HSP3_ROOT\\common\\" $makeScript
    if (!$?) {
        exit 1
    }
} finally {
    cd $workDir
}
