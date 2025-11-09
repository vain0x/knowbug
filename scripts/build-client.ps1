# knowbug_client をビルドする。

# 注意: pwsh で GUI アプリを起動するとき、終了を待機しないことがある。
#      アプリをパイプの途中で実行するときは待機されるので、
#      xxx.exe (GUIアプリ) が終了するのを待ちたいときは xxx.exe | out-null と書く。

$baseDir = (get-item .).fullName
$clientHspRoot = "$PWD/bin/client"
$serverHspRoot = "$PWD/bin/server"

./scripts/hsp3-make.ps1
if (!$?) {
    write-error 'hsp3-make.ps1 が失敗しました。'
    exit 1
}

try {
    chdir "$baseDir/src/knowbug_client"

    # クライアントをビルドする。
    & "$clientHspRoot/hsp3_make.exe" "kc_main.hsp" "$baseDir/src/knowbug_client" $clientHspRoot | out-null
    if (!$?) {
        write-error 'knowbug_client のビルドに失敗しました。'
        exit 1
    }

    echo 'knowbug_client の実行ファイルを生成しました。'

    # プロキシをビルドする。
    & "$clientHspRoot/hsp3_make.exe" "kc_main_proxy.hsp" "$baseDir/src/knowbug_client" $clientHspRoot | out-null
    if (!$?) {
        write-error 'knowbug_client_proxy のビルドに失敗しました。'
        exit 1
    }

    # プロキシの設定ファイルを作成する。
    $proxyConfigPath = "$serverHspRoot/knowbug_client_proxy.txt"
    echo "$baseDir/src/knowbug_client/kc_main.hsp" >$proxyConfigPath
    echo $clientHspRoot >>$proxyConfigPath

    # プロキシを配置する。
    copy-item -force "$baseDir/src/knowbug_client/knowbug_client_proxy.exe" "$serverHspRoot/knowbug_client.exe"

    echo 'knowbug_client_proxy の実行ファイルを作成しました。'
} finally {
    chdir $baseDir
}
