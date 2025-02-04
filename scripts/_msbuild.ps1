# MSBuild (Visual Studio のビルドを行うツール) へのパスを取得する
function getMSBuild() {
    # パスが通っている場合、ファイルパスを返す
    $it = $(Get-Command 'MSBuild.exe' -ErrorAction Ignore)
    if ($it) {
        return $it.Path
    }

    # Visual Studio 2022 がインストールされている場合
    # (既定のインストール先にファイルが存在すれば、そのパスを返す)
    $it = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe'
    if (Test-Path $it) {
        return $it
    }

    Write-Error 'MSBuild.exe が見つかりません。MSBuild.exe へのパスを通すか、Build Tools for Visual Studio 2022 をインストールしてください'
    exit 1
}
