$CMAKE_URL = "https://cmake.org/files/v3.11/cmake-3.11.4-win32-x86.zip"
$CMAKE_ZIP = ($CMAKE_URL -split "/")[-1]
$CMAKE_DIR = [System.IO.Path]::GetFileNameWithoutExtension($CMAKE_ZIP)

$CMAKE_ZIP = "download\cmake\" + $CMAKE_ZIP

echo $CMAKE_URL
echo $CMAKE_ZIP
echo $CMAKE_DIR

# TLS1.2 を有効にする (cmake.orgは TLS1.2)
# [Net.ServicePointManager]::SecurityProtocol
# を実行して "Ssl3, Tls" と出た場合、TLS1.2は無効となっている
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.SecurityProtocolType]::Tls12;

# 展開済みフォルダがある?
if((Test-Path $CMAKE_DIR) -eq $true) {
	# 削除する
	Remove-Item -Recurse -Force $CMAKE_DIR
	# 終了する
	#exit
}

# ダウンロードする
if((Test-Path $CMAKE_ZIP) -eq $false) {
	if((Test-Path "download\cmake") -ne $true) {
		mkdir "download\cmake"
	}
	wget $CMAKE_URL -Outfile $CMAKE_ZIP
}

# 展開する
Expand-Archive $CMAKE_ZIP -DestinationPath .
