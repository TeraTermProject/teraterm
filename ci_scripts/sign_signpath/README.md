# SignPath によるコード署名

- [SignPath][SignPath]

## 概要

TeraTermポータブル版zipから署名済みTeraTermポータブル版zipとインストーラーを作成します。

スクリプト(sign_signpath.ps1)を実行すると
[SignPath を利用するためのモジュール](https://about.signpath.io/documentation/powershell/)が
インストールされ、署名済みファイルが作成されます。

## スクリプトの使い方

準備

- ポータブル版zip
  - 例 `teraterm-5.5.0.zip`
- [SignPath][SignPath] の API token
  - `SIGNPATH_API_TOKEN` 環境変数にセットしておく
  - `set SIGNPATH_API_TOKEN=AD50kRDEwv...` (cmd)
  - `$env:SIGNPATH_API_TOKEN = ""LKF765DSFlkj...."""` (PowerShell)
  - `export SIGNPATH_API_TOKEN=JKLBLID.......` (bash)

実行

(例)

    pwsh sign_signpath.ps1 ../../installer/Output/teraterm-5.5.0.zip

出力

- 署名済みポータブル版zip
  - 例 `teraterm-5.5.0_signed.zip`
- 署名済みインストーラー
  - 例 `teraterm-5.5.0_signed.exe`

## 実行例

```
> ver

Microsoft Windows [Version 10.0.19045.5917]

> pwsh -v
PowerShell 7.5.1

> set SIGNPATH_API_TOKEN=AD50kRD.........

> pwsh sign_signpath.ps1 ../../installer/Output/teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot.zip
pwd: "C:\...\teraterm\ci_scripts\sign_signpath"
Source directory: C:\...\teraterm\ci_scripts\sign_signpath/../..
zipFilename(Portable unsigned zip): "../../installer/Output/teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot.zip"
version: "5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot"

    Directory: C:\...\teraterm\ci_scripts\sign_signpath
 :
 :
フォルダ名 : teraterm-5.5.0-dev-20250605_162051-d25685f9e-noname-snapshot
SHA256 hash: EC8A0E32C6FFD09AB2AE8BCB6FA630E191D30A7A36CF1A834B0CC522BD28DBA9
Submitted signing request at 'https://app.signpath.io/api/v1/78e41f48-55fc-4f33-8ea5-08ac32f65ac8/SigningRequests/4db21f6f-76e4-4a95-9d56-ef98a85be74e'
Checking status... InProgress
Checking status... InProgress
Checking status... Completed
Downloading signed artifact...
Downloaded signed artifact and saved at 'C:\...\teraterm\ci_scripts\sign_signpath\teraterm_signed.zip'
4db21f6f-76e4-4a95-9d56-ef98a85be74e
 :
 :
Inno Setup 6 Command-Line Compiler
Copyright (C) 1997-2025 Jordan Russell. All rights reserved.
Portions Copyright (C) 2000-2025 Martijn Laan. All rights reserved.
Portions Copyright (C) 2001-2004 Alex Yackimoff. All rights reserved.
 :
 :
Successful compile (5.125 sec). Resulting Setup program filename is:
C:\...\teraterm\ci_scripts\sign_signpath\teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot_unsigned.exe
Requesting signing for teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot.exe
SHA256 hash: 37A3F1AEA3428170421B0E4249ABE65C8B2296A4F5BCEADA95CD54947726A674
Submitted signing request at 'https://app.signpath.io/api/v1/78e41f48-55fc-4f33-8ea5-08ac32f65ac8/SigningRequests/7c7795fd-af3f-4177-b9f5-e71e7b0a2bcf'
Checking status... Completed
Downloading signed artifact...
Downloaded signed artifact and saved at 'C:\...\teraterm\ci_scripts\sign_signpath\teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot_signed.exe'
7c7795fd-af3f-4177-b9f5-e71e7b0a2bcf

> dir /B *.zip *.exe
teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot_signed.zip
teraterm-5.5.0-dev-20250699_999999-d25685f9e-noname-snapshot_signed.exe

>

```

[SignPath]:https://signpath.io/
