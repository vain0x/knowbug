# hsp3_make.hsp をコンパイルする。
# HSP スクリプトの実行ファイルを作成するためのスクリプト。

$workDir = (get-item .).fullName
$clientHspRoot = "$PWD/bin/client"

# 実行ファイル生成用のスクリプトをコンパイルする。
& "$clientHspRoot/hspcmp.exe" "--compath=$clientHspRoot/common/" "$workDir/scripts/hsp3_make_cli.hsp"
if (!$?) {
    write-error 'hsp3_make_cli.hsp のコンパイルに失敗しました。'
    exit 1
}

# AXファイルが生成されたことを確認する。
$makeAx = "$workDir/scripts/hsp3_make_cli.ax"
if (!(test-path $makeAx)) {
    write-error "$makeAx が見つかりません。"
    exit 1
}

# 実行ファイルを生成するための実行ファイルを生成する。
& "$clientHspRoot/hsp3cl.exe" $makeAx "$workDir/scripts/hsp3_make.hsp" $clientHspRoot $clientHspRoot
if (!$?) {
    write-error 'hsp3_make.hsp の実行ファイル生成に失敗しました。'
    exit 1
}
