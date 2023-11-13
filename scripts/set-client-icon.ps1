# knowbug_client のアイコンを変更する。
# knowbug_client のビルドが完了した後に実行すること。

$workDir = (get-item .).fullName
$clientHspRoot = "$PWD/bin/client"

# アイコンを変更する。
# 参考: http://dev.onionsoft.net/trac/openhsp/browser/trunk/tools/win32/hsed3_footy2/PackIconResource.cpp
cmd /c "`"$clientHspRoot/iconins.exe`" -e`"$workDir/src/knowbug_client/knowbug_client.exe`" -i`"$workDir/src/knowbug_client/knowbug_client.ico`""
if (!$?) {
    write-error 'knowbug_client のアイコン変更に失敗しました。'
    exit 1
}

echo 'knowbug_client のアイコンを変更しました。'
