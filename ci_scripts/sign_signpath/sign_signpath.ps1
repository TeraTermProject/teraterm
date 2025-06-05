#
# SignPathを使ってTera Termのポータブルzipとインストーラを署名
#
# - 準備
#   - 環境変数 SIGNPATH_API_TOKEN に SignPath の API Token をセットしておく
# - 実行(例)
#   - pwsh sign_signpath.ps1 ../../installer/Output/teraterm-5.5.0.zip
#
param(
    [string]$zipFilename
)

if (-not $zipFilename) {
    throw "ex  sign_signpath.ps1 teraterm-5.5.0.zip"
    #$zipFilename = Get-ItemProperty -Path ".\tera*.zip" | Sort-Object -Descending | Select-Object -First 1
}
$API_TOKEN = $env:SIGNPATH_API_TOKEN
if (-not $API_TOKEN) {
    Write-Output "SIGNPATH_API_TOKEN is not set."
    Write-Output "Set SIGNPATH_API_TOKEN environment variable to your SignPath API token."
    Write-Output "For example, in PowerShell: `$env:SIGNPATH_API_TOKEN = ""LKF765DSFlkj...."""
    # string "Bearer " is unnecessary.
    throw "SIGNPATH_API_TOKEN is not set."
}

$SOURCE_DIR = Join-Path $PSScriptRoot "../..";
$ORGANIZATION_ID = "78e41f48-55fc-4f33-8ea5-08ac32f65ac8"
$PROJECT_SLUG = "teraterm"
$SIGNING_POLICY_SLUG = "test-signing"
$ISCC = Join-Path $SOURCE_DIR "buildtools/innosetup6/ISCC.exe"
$version = $zipFilename -replace '^.*teraterm-', '' -replace '\.zip$', ''

# debug
$enableSignZip = $true
$enableSignInstaller = $true

#Set-Location "$PSScriptRoot"
Write-Output "pwd: ""$PSScriptRoot"""
Write-Output "Source directory: $SOURCE_DIR"
Write-Output "zipFilename(Portable unsigned zip): ""$zipFilename"""
Write-Output "version: ""$version"""
#Write-Output "API_TOKEN: ""$API_TOKEN"""

#
# $zipFilename を $destination フォルダへ展開する
#
# $zipFilename 内のファイルは、フォルダの下に入っていることを想定している
#
#   teraterm-5.5.0-dev-20250526_234540-e7d9a3fb0-noname-snapshot/ttermpro.exe
#   teraterm-5.5.0-dev-20250526_234540-e7d9a3fb0-noname-snapshot/keycode.exe
#     :
#     v
#   $destination/ttermpro.exe
#   $destination/keycode.exe
#     :
#
function ExtractTeraTermPortableZip() {
    param (
        [string]$zipFilename,
        [string]$destination
    )

    # 展開先
    if (Test-Path $destination) {
        Write-Error "remove ""$destination"""
        exit 1
    }
    mkdir $destination

    # 一時的に展開するフォルダ
    $unzip_tmp = "unzip_tmp_{0}" -f ([System.Guid]::NewGuid().ToString("N"))

    Expand-Archive -Path $zipFilename -DestinationPath $unzip_tmp -Force

    $dir = Get-ChildItem -Path $unzip_tmp
    Write-Output "フォルダ名 : $($dir.Name)"

    # $dir以下のファイルとフォルダをすべて$destinationへ移動
    foreach ($item in Get-ChildItem -Path $dir) {
        Move-Item -Path $item.FullName -Destination $destination -Force
    }

    Remove-Item -Path $unzip_tmp -Recurse -Force
}

#
# ポータブルzip($unsignedZipFilename) を署名して $signedZipFilename へ書き込む
#  zip内の指定ファイルを署名する
#  どのファイルを署名するかは SignPathサイト内で設定している
#    Project / Artifact Configuration / teraterm_portable
#
function SignZip {
    param (
        [string]$unsignedZipFilename,
        [string]$signedZipFilename,
        [string]$API_TOKEN
    )

    if ($enableSignZip) {
        Submit-SigningRequest `
          -InputArtifactPath $unsignedZipFilename `
          -ApiToken $API_TOKEN `
          -ArtifactConfigurationSlug "teraterm_portable" `
          -OrganizationId $ORGANIZATION_ID `
          -ProjectSlug $PROJECT_SLUG `
          -SigningPolicySlug $SIGNING_POLICY_SLUG `
          -OutputArtifactPath $signedZipFilename `
          -Force `
          -WaitForCompletion
    } else {
        Copy-Item $unsignedZipFilename $signedZipFilename
    }
}

#
# 未署名インストーラ($unsignedInstaller) を署名して $signedInstaller へ書き込む
#
function SignInstaller {
    param (
        [string]$unsignedInstaller,
        [string]$signedInstaller,
        [string]$API_TOKEN
    )

    if ($enableSignInstaller) {
        Write-Output "Requesting signing for teraterm-${version}.exe"
        Submit-SigningRequest `
          -InputArtifactPath "$unsignedInstaller" `
          -ApiToken $API_TOKEN `
          -ArtifactConfigurationSlug "installer" `
          -OrganizationId $ORGANIZATION_ID `
          -ProjectSlug $PROJECT_SLUG `
          -SigningPolicySlug $SIGNING_POLICY_SLUG `
          -OutputArtifactPath "$signedInstaller" `
          -Force `
          -WaitForCompletion
    } else {
        Copy-Item "$unsignedInstaller" "$signedInstaller"
    }
}

#
# 未署名のインストーラ作成
#
function CreateUnsignedInstaller
{
    param (
        [string]$baseIssFilename,
        [string]$version,
        [string]$WorkDir
    )

    Push-Location $WorkDir
    $issFilename = "teraterm.iss"
    (Get-Content $baseIssFilename) `
      -replace 'LicenseFile=release\\license.txt', "LicenseFile=teraterm\license.txt" `
      -replace 'Source: \.\.\\teraterm\\release', "Source: teraterm" `
      -replace 'Source: release', "Source: teraterm" `
      -replace 'Source: \.\.\\doc\\en', "Source: teraterm" `
      -replace 'Source: \.\.\\doc\\ja', "Source: teraterm" `
      -replace 'Source: \.\.\\ttssh2\\ttxssh\\Release', "Source: teraterm" `
      -replace 'Source: \.\.\\cygwin\\cygterm\\cygterm\+-x86_64', "Source: teraterm" `
      -replace 'Source: \.\.\\cygwin\\cygterm', "Source: teraterm" `
      -replace 'Source: \.\.\\cygwin\\Release', "Source: teraterm" `
      -replace 'Source: \.\.\\ttpmenu\\Release', "Source: teraterm" `
      -replace 'Source: \.\.\\ttpmenu\\readme.txt', "Source: teraterm\ttmenu_readme-j.txt" `
      -replace 'Source: \.\.\\TTProxy\\Release', "Source: teraterm" `
      -replace 'Source: \.\.\\TTXKanjiMenu\\release', "Source: teraterm" `
      -replace 'Source: \.\.\\TTXSamples\\release', "Source: teraterm" `
      | Set-Content $issFilename

    & "$ISCC" "$issFilename" /DAppVer=${version} /O..
    remove-Item -Path $issFilename -Force
    Pop-Location
}

if (-not (Test-Path $zipFilename)) {
    Write-Error "Tera Term portable zip file not found: $zipFilename"
    exit 1
}

if (Test-Path "teraterm_unsigned.zip") {
    Remove-Item "teraterm_unsigned.zip"
}
if (Test-Path "teraterm-${version}_signed.zip") {
    Remove-Item "teraterm-${version}_signed.zip"
}

# SignPath moduleをインストール
if (-not (Get-Module -ListAvailable -Name SignPath)) {
    Install-Module -Name SignPath -Force -Scope CurrentUser
}

# 作業用フォルダ
$WorkDir = "sign_signpath_{0}" -f ([System.Guid]::NewGuid().ToString("N"))
$WorkDirTT = Join-Path $WorkDir "teraterm"

# teraterm_unsigned.zip を作る
#  作ったzip内のフォルダは "teraterm/" になっている
ExtractTeraTermPortableZip -zipFilename $zipFilename -destination $WorkDirTT
try {
    Compress-Archive -Path $WorkDirTT -DestinationPath "teraterm_unsigned.zip"
} catch {
    Write-Error """teraterm_unsigned.zip"" error: $_"
    exit 1
}
Remove-Item -Path $WorkDirTT -Recurse -Force

# teraterm_unsigned.zip を署名 teraterm_signed.zip を作る
SignZip `
  -unsignedZipFilename "teraterm_unsigned.zip" `
  -signedZipFilename "teraterm_signed.zip" `
  -API_TOKEN $API_TOKEN
remove-Item -Path "teraterm_unsigned.zip" -Force

# 署名済みのポータブルzipを "teraterm/" へ展開
ExtractTeraTermPortableZip -zipFilename "teraterm_signed.zip" -destination $WorkDirTT
remove-Item -Path "teraterm_signed.zip" -Force

# フォルダ名を変更して teraterm-${version}_signed.zip を作成
$WorkDirTTV = Join-Path $WorkDir "teraterm-${version}"
Copy-Item -Path $WorkDirTT -Destination $WorkDirTTV -Recurse -Force
Compress-Archive -Path $WorkDirTTV -DestinationPath "teraterm-${version}_signed.zip"
Remove-Item -Path $WorkDirTTV -Recurse -Force

# 未署名インストーラ作成
#  (インストーラー本体が未署名、内部にアーカイブされたファイルは署名済み)
CreateUnsignedInstaller `
  -baseIssFilename "$SOURCE_DIR/installer/teraterm.iss" `
  -version ${version}_unsigned `
  -WorkDir $WorkDir
Remove-Item -Path $WorkDir -Recurse -Force

# インストーラを署名する
SignInstaller `
  -unsignedInstaller "teraterm-${version}_unsigned.exe" `
  -signedInstaller "teraterm-${version}_signed.exe" `
  -API_TOKEN $API_TOKEN
Remove-Item -Path "teraterm-${version}_unsigned.exe" -Force
