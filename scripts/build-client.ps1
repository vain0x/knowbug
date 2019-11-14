# knowbug_client をビルドする。

if (!$env:HSP3_ROOT) {
    write-error '環境変数 HSP3_ROOT を設定してください。'
    exit 1
}

if (!$env:HSP3_ROOT_DEV) {
    write-error '環境変数 HSP3_ROOT_DEV を設定してください。'
    exit 1
}

./scripts/hsp3-make.ps1
if (!$?) {
    write-error 'hsp3-make failed'
    exit 1
}

$workDir = (get-item .).fullName
try {
    $makeAx = "$workDir/scripts/hsp3_make.ax"
    $makeFile = "$workDir/src/knowbug_client/hsp3_makefile"
    if (!(test-path $makeAx)) {
        write-error 'hsp3_make.ax が見つかりません。'
        exit 1
    }

    # クライアントをビルドする。
    cd "$workDir/src/knowbug_client"
    echo "$workDir/src/knowbug_client/kc_main.hsp" >$makeFile
    echo $env:HSP3_ROOT >>$makeFile
    & "$env:HSP3_ROOT/hsp3.exe" $makeAx
    if (!$?) {
        write-error 'クライアントのビルドに失敗しました。'
        exit 1
    }
    sleep 1

    # プロキシをビルドする。
    cd "$workDir/src/knowbug_client"
    echo "$workDir/src/knowbug_client/kc_main_proxy.hsp" >$makeFile
    echo $env:HSP3_ROOT >>$makeFile
    & "$env:HSP3_ROOT/hsp3.exe" $makeAx
    if (!$?) {
        write-error 'プロキシクライアントのビルドに失敗しました。'
        exit 1
    }

    # プロキシの設定ファイルを作成する。
    $proxyConfigPath = "$env:HSP3_ROOT/knowbug_client_proxy.txt"
    echo "$workDir/src/knowbug_client/kc_main.hsp" >$proxyConfigPath
    echo $env:HSP3_ROOT_DEV >>$proxyConfigPath

    # プロキシを配置する。
    copy-item -force "$workDir/src/knowbug_client/knowbug_client_proxy.exe" "$env:HSP3_ROOT/knowbug_client.exe"
} finally {
    cd $workDir
}
