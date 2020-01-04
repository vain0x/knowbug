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

$workDir = (get-item .).fullName
$serverHspRoot = (get-item $env:KNOWBUG_SERVER_HSP3_ROOT).fullName
$clientHspRoot = (get-item $env:KNOWBUG_CLIENT_HSP3_ROOT).fullName

./scripts/hsp3-make.ps1
if (!$?) {
    write-error 'hsp3-make.ps1 が失敗しました。'
    exit 1
}

# クライアントをビルドする。
& "$clientHspRoot/hsp3_make.exe" "$workDir/src/knowbug_client/kc_main.hsp" "$workDir/src/knowbug_client" $clientHspRoot | out-null
if (!$?) {
    write-error 'knowbug_client のビルドに失敗しました。'
    exit 1
}

# アイコンを変更する。
# 参考: http://dev.onionsoft.net/trac/openhsp/browser/trunk/tools/win32/hsed3_footy2/PackIconResource.cpp
cmd /c "`"$clientHspRoot/iconins.exe`" -e`"$workDir/src/knowbug_client/knowbug_client.exe`" -i`"$workDir/src/knowbug_client/knowbug_client.ico`""
if (!$?) {
    write-error 'knowbug_client のアイコン変更に失敗しました。'
    exit 1
}

# プロキシをビルドする。
& "$clientHspRoot/hsp3_make.exe" "$workDir/src/knowbug_client/kc_main_proxy.hsp" "$workDir/src/knowbug_client" $clientHspRoot | out-null
if (!$?) {
    write-error 'knowbug_client_proxy のビルドに失敗しました。'
    exit 1
}

# プロキシの設定ファイルを作成する。
$proxyConfigPath = "$serverHspRoot/knowbug_client_proxy.txt"
echo "$workDir/src/knowbug_client/kc_main.hsp" >$proxyConfigPath
echo $clientHspRoot >>$proxyConfigPath

# プロキシを配置する。
copy-item -force "$workDir/src/knowbug_client/knowbug_client_proxy.exe" "$serverHspRoot/knowbug_client.exe"
