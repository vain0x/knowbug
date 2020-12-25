#!/bin/pwsh
# HSP3 スクリプトをデバッグ環境で実行する。

# 使い方
#   ./scripts/run <script> <runtime-name>

$workDir = (get-item .).fullName
$hspRoot = "$PWD/bin/server"

$scriptName = (get-item $args[0]).fullName
$runtimeName = $args[1]

$axName = [system.io.path]::changeExtension($scriptName, ".ax")
$baseDir = [system.io.path]::getDirectoryName((get-item $scriptName).fullName)

if (!$hspRoot) {
    write-error '環境変数 KNOWBUG_SERVER_HSP3_ROOT を設定してください。'
    exit 1
}

if (!$scriptName) {
    write-error "コンパイルするファイル名を引数に指定してください。"
    exit 1
}

if ($runtimeName -eq "hsp3") {
    $compilerArgs = @("-d", "-w")
} elseif ($runtimeName -eq "hsp3utf") {
    $compilerArgs = @("-d", "-w", "-i", "-u")
} else {
    write-error "ランタイムが不明です。hsp3 か hsp3utf を指定してください。"
    exit 1
}

try {
    cd $baseDir
    & "$hspRoot/hspcmp" "--compath=$hspRoot/common/" $compilerArgs $scriptName
    if (!$?) {
        exit 1
    }

    & "$hspRoot/$runtimeName" $axName
} finally {
    cd $workDir
}
