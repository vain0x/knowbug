# HSP3 スクリプトを UTF-8 版ランタイムで実行する

# 使い方
#   ./scripts/run-utf8 hoge.hsp

$H = $env:HSP3_ROOT
$workDir = (get-item .).fullName

$scriptName = $args[0]
$axName = [system.io.path]::changeExtension($scriptName, ".ax")
$baseDir = [system.io.path]::getDirectoryName((get-item $scriptName).fullName)

if (!$H) {
    write-error "環境変数 HSP3_ROOT を設定してください。"
    exit 1
}

if (!$scriptName) {
    write-error "コンパイルするファイル名を引数に指定してください。"
    exit 1
}

try {
    cd $baseDir
    & "$H/hspcmp" --compath="$H/common/" -d -w -i -u $scriptName
    if (!$?) {
        exit 1
    }

    & "$H/hsp3utf" $axName
} finally{
    cd $workDir
}
