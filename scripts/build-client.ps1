# knowbug_client をビルドする。
# 注意: pwsh で GUI アプリを起動するとき、終了を待機しないことがある。
#      アプリをパイプの途中で実行するときは待機されるので、
#      xxx.exe (GUIアプリ) が終了するのを待ちたいときは xxx.exe | out-null と書く。

if (!$env:KNOWBUG_SERVER_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_SERVER_HSP3_ROOT を設定してください。'
    exit 1
}

if (!$env:KNOWBUG_CLIENT_HSP3_ROOT) {
    write-error '環境変数 KNOWBUG_CLIENT_HSP3_ROOT を設定してください。'
    exit 1
}

./scripts/hsp3-make.ps1
if (!$?) {
    write-error 'hsp3-make.ps1 が失敗しました。'
    exit 1
}

$workDir = (get-item .).fullName
try {
    $makeAx = "$workDir/scripts/hsp3_make.ax"
    $makeFile = "$workDir/src/knowbug_client/hsp3_makefile"
    if (!(test-path $makeAx)) {
        write-error "$makeAx が見つかりません。"
        exit 1
    }

    # クライアントをビルドする。
    cd "$workDir/src/knowbug_client"
    echo "$workDir/src/knowbug_client/kc_main.hsp" >$makeFile
    echo $env:KNOWBUG_SERVER_HSP3_ROOT >>$makeFile
    & "$env:KNOWBUG_SERVER_HSP3_ROOT/hsp3.exe" $makeAx | out-null
    if (!$?) {
        write-error 'knowbug_client のビルドに失敗しました。'
        exit 1
    }

    # プロキシをビルドする。
    cd "$workDir/src/knowbug_client"
    echo "$workDir/src/knowbug_client/kc_main_proxy.hsp" >$makeFile
    echo $env:KNOWBUG_SERVER_HSP3_ROOT >>$makeFile
    & "$env:KNOWBUG_SERVER_HSP3_ROOT/hsp3.exe" $makeAx | out-null
    if (!$?) {
        write-error 'knowbug_client_proxy のビルドに失敗しました。'
        exit 1
    }

    # プロキシの設定ファイルを作成する。
    $proxyConfigPath = "$env:KNOWBUG_SERVER_HSP3_ROOT/knowbug_client_proxy.txt"
    echo "$workDir/src/knowbug_client/kc_main.hsp" >$proxyConfigPath
    echo $env:KNOWBUG_CLIENT_HSP3_ROOT >>$proxyConfigPath

    # プロキシを配置する。
    copy-item -force "$workDir/src/knowbug_client/knowbug_client_proxy.exe" "$env:KNOWBUG_SERVER_HSP3_ROOT/knowbug_client.exe"
} finally {
    cd $workDir
}
