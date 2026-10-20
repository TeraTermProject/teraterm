# ttlaunch

Tera Term(ttermpro.exe) を shell のターミナルとして起動するランチャー

実行するとマウスカーソル位置にポップアップメニューが表示され、
起動する shell を選択できます。

## コマンドラインオプション

```
ttlaunch [options] [cygterm options]
```

- `-d <directory>`
  - shell を起動するフォルダ
  - Cygwin, MSYS2, Git Bash では環境変数 `CHERE_INVOKING=y` も設定します(下記参照)
- `-v <NAME=VALUE>`
  - shell に渡す環境変数、複数指定できます
- `-e <id>`
  - ポップアップメニューを表示せずに起動します
- `-setup`
  - 設定ダイアログを開きます(下記参照)
- `-h`
  - ヘルプと、この環境で使用できる id を表示します

その他のオプションは CygTerm+ (cygterm または msys2term) へ渡されます。

`-e` で使用できる id は次の通りです。
`cygwin`, `msys2`, `mingw32`, `mingw64`, `ucrt64`, `clang64`, `clangarm64`,
`gitbash`, `wsl`, `wsl:<ディストリビューション名>`, `cmd`, `powershell`, `pwsh`

例

```
ttlaunch -d C:\work
ttlaunch -e mingw64 -v LANG=ja_JP.UTF-8
ttlaunch -e wsl:Ubuntu-24.04
```

### Git Bash, MSYS2 から起動するときの注意

Git Bash, MSYS2 の shell から ttlaunch.exe を起動すると、
`/` で始まる引数は Windows のパスに変換されます。
例えば `-s /bin/sh` は `-s "C:/Program Files/Git/usr/bin/sh"` に変換され、
cygterm, msys2term で shell を起動できません。
`-v HOME=/home/user` のような `/` で始まる値も同様です。

コマンドプロンプトや PowerShell から起動するか、次のように変換を止めてください。

```
MSYS2_ARG_CONV_EXCL='*' ./ttlaunch -e cygwin -s /bin/sh
```

## shellの起動

ttlaunch.exe を起動(エクスプローラーからダブルクリック)すると、シェルの一覧と
`設定ダイアログを開く`ポップアップが出ます。

ttlaunch.exe へのショートカットに `-d` をつけて、エクスプローラーからフォルダをドロップすると
ポップアップからドロップしたフォルダに shell を開くことができます。

コマンドラインから `ttlaunch -d .` とすることでも shell を開くことができます。

設定ダイアログでレジストリを登録すると、
エクスプローラのコンテキストメニューから shell を開くことができます。

## 設定ダイアログ

`ttlaunch -setup` を実行するか、ポップアップメニューの `Open settings dialog...` を選択すると
設定ダイアログが開きます。次のことができます。

- メニューの種類の選択
  - Popup menu: "Open in Tera Term" の 1 項目を登録します。選択すると ttlaunch のポップアップメニューが表示されます。
    登録内容が固定なので、shell の追加・削除は次の起動から反映されます
  - Items: 各 shell をコンテキストメニューの項目として直接登録します(サブメニューなし)。
    項目名は `Tera Term: Cygwin` のようになります。Windows XP / Vista でも使えますが、メニューが shell の数だけ長くなります
  - Submenu: "Open in Tera Term" のサブメニューに、ポップアップメニューと同じ項目(各 shell)を並べます。
    Windows 7 以降で使用できます
  - Items と Submenu の各項目は `ttlaunch -e <id> -d "%V"` を実行します。
    末尾に設定ダイアログを開く項目(`ttlaunch -setup`)も登録します
- 出力先の選択
  - Registry (HKEY_CURRENT_USER): Register / Unregister で直接レジストリを変更します
  - .reg file: Register / Unregister と同じ内容の reg ファイルを書き出します(ボタンは Save (register)... / Save (unregister)... になります)。
    ダブルクリックすると HKEY_CURRENT_USER に反映されます
- Register: 既存の登録(HKEY_CURRENT_USER の `ttlaunch`, `ttlaunch_*`)をすべて削除してから、選択した種類で登録します
- Unregister: HKEY_CURRENT_USER の登録をすべて削除します
- Registry Editor の Open...: レジストリ エディタで登録先のキー(HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell)を開きます。
  起動時に管理者権限の確認(UAC)が表示されます

Items と Submenu の内容は、設定ダイアログを開いた時点で見つかった環境から作成します。
Cygwin や WSL ディストリビューションを追加・削除したときは、登録し直してください。

HKEY_LOCAL_MACHINE に登録されたもの(他のユーザーや管理者が登録したもの)は設定ダイアログからは削除できません。
HKEY_LOCAL_MACHINE に登録があるときは、状態の 2 行目にその旨を表示します。

設定ダイアログの文言と、ポップアップメニューの `Open settings dialog...`、起動できる項目が無いときの文言は、
ttlaunch.exe と同じフォルダの ttlaunch.lng から読み込みます。
ttlaunch.lng は言語ごとのセクション(`[English]`, `[Japanese]` など)に文言を持ち、
Tera Term の設定(TERATERM.INI の UILanguageFile)で選ばれている言語のセクションを使用します
(言語名は Tera Term の lng ファイルの `[Info]` の `language=` の括弧より前の部分です)。
セクションやキーが無いときは英語になります。
ソースの ttlaunch.lng は UTF-8 で、ビルド時に UTF-16LE に変換して配置します。

regファイルはBOMつきUTF-16LEです。

## メニューの項目

インストールされている環境を調べて、メニューを作成します。

- Cygwin
  - TERATERM.INI の CygwinDirectory、無い場合はレジストリ HKLM(HKCU)\SOFTWARE\Cygwin\setup の rootdir、
    無い場合は固定ドライブの \cygwin64, \cygwin
  - アイコンは Cygwin フォルダの Cygwin.ico
- MSYS2 (MSYS, MinGW x32, MinGW x64, UCRT64, CLANG64, CLANGARM64)
  - 固定ドライブの \msys64, \msys
  - 各環境のフォルダ(mingw64 など)が存在するものだけ表示します
  - アイコンは MSYS2 フォルダの msys2.ico, mingw32.ico, mingw64.ico, ucrt64.ico, clang64.ico, clangarm64.ico
- Git Bash
  - レジストリ HKLM(HKCU)\SOFTWARE\GitForWindows の InstallPath、無い場合は固定ドライブの \Program Files\Git
  - アイコンは git-bash.exe のアイコン
- WSL
  - デフォルトのディストリビューション
  - インストール済みのディストリビューション (docker-desktop* は除く)
  - アイコンは wsl.exe のアイコン
- Windows の shell
  - コマンドプロンプト (cmd)
  - Windows PowerShell
  - PowerShell 7 (pwsh.exe が PATH にある場合)
  - アイコンは cmd.exe, powershell.exe, pwsh.exe のアイコン
  - Microsoft Store 版の PowerShell 7 の pwsh.exe はアプリ実行エイリアス(0 バイトの再解析ポイント)で
    アイコンを読めないため、再解析ポイントが指す実体(C:\Program Files\WindowsApps\...\pwsh.exe)からアイコンを取得します

アイコンはファイルが見つかった場合だけ表示します。

Tera Term 単体では Windows のコンソールプログラムを実行できないため、
CygTerm+ を使用して shell を実行します。
CygTerm+ には Cygwin 用の cygterm.exe と、MSYS2 用の msys2term.exe があります。
以下、それぞれ cygterm, msys2term と記述します。

cygterm の実行には Cygwin、msys2term の実行には MSYS2, Git for Windows のいずれかが
インストールされている必要があります
([使用する DLL と実際のパス](#使用する-dll-と実際のパス) 参照)。

ttlaunch.exe は cygterm.exe, msys2term.exe と同じフォルダに置いてください。

## Cygwin, MSYS2, Git Bash で起動する shell

ttlaunch は shell を指定しません。cygterm, msys2term は次のように shell を決めます。

- 設定ファイル(cygterm.cfg, msys2term.cfg)の `SHELL = auto` のとき、ユーザー情報(getpwnam(3))のログイン shell を使用します
  - /etc/passwd にユーザーのエントリがあればその shell、無い場合は /etc/nsswitch.conf の `db_shell` で決まります
  - 通常は bash (`/bin/bash`, `/usr/bin/bash`) です
- 設定ファイルの `LOGIN_SHELL = Yes` により、ログイン shell として起動します
  - /etc/profile が読み込まれます(MSYS2 では `MSYSTEM` による設定が行われます)

| 環境     | 使用するプログラム | 設定ファイル  | /etc/nsswitch.conf の db_shell (デフォルト) |
|----------|--------------------|---------------|---------------------------------------------|
| Cygwin   | cygterm            | cygterm.cfg   | 指定なし(`/bin/bash`)                       |
| MSYS2    | msys2term          | msys2term.cfg | `cygwin desc`                               |
| Git Bash | msys2term          | msys2term.cfg | `env windows`                               |

`db_shell` には shell を探す方法(スキーマ)を順に指定します。
`cygwin` は Active Directory の属性、`desc` は Windows のユーザーアカウントの説明欄の指定を使用します。
`windows` は無視されます。
どのスキーマでも見つからない場合は `/bin/bash` になります。
詳しくは [Cygwin User's Guide](https://cygwin.com/cygwin-ug-net/ntsec.html) を参照してください。

別の shell を使用する場合は、次のいずれかで指定できます。

- 設定ファイルの `SHELL` に指定する
- /etc/passwd または /etc/nsswitch.conf の `db_shell` を変更する
- ttlaunch の引数に `-s /usr/bin/zsh` などを指定する(cygterm, msys2term へ渡されます)

### 使用する DLL と実際のパス

cygterm は cygwin1.dll、msys2term は msys-2.0.dll を使用するプログラムです。
ttlaunch は、使用する DLL のあるフォルダを環境変数 PATH の先頭に追加して cygterm, msys2term を起動します。
Git Bash も msys2term を使用しますが、Git for Windows の msys-2.0.dll を使用します。

| 環境     | 使用するプログラム | 使用する DLL (デフォルト)                 | `/` の実際のパス     |
|----------|--------------------|-------------------------------------------|----------------------|
| Cygwin   | cygterm            | C:\cygwin64\bin\cygwin1.dll               | C:\cygwin64          |
| MSYS2    | msys2term          | C:\msys64\usr\bin\msys-2.0.dll            | C:\msys64            |
| Git Bash | msys2term          | C:\Program Files\Git\usr\bin\msys-2.0.dll | C:\Program Files\Git |

`/` の実際のパスは、使用した DLL の場所で決まります。
そのため /etc/passwd, /etc/nsswitch.conf, /etc/profile なども、環境ごとに実際のパスが変わります。
ホームディレクトリ(~)も環境ごとに異なり、/etc/nsswitch.conf の `db_home` などで決まります。
例えば /etc/profile は次のファイルです。

- Cygwin: C:\cygwin64\etc\profile
- MSYS2: C:\msys64\etc\profile
- Git Bash: C:\Program Files\Git\etc\profile

cygterm, msys2term が読み込む設定ファイルも同様です。
cygterm は /etc/cygterm.conf, $HOME/.cygtermrc、
msys2term は /etc/msys2term.conf, $HOME/.msys2termrc を読み込みます。
$HOME は Windows の環境変数 HOME が設定されていればその値、無い場合は `db_home` などで決まります。

## MSYS2 の環境変数 MSYSTEM

MSYS2 の各環境は、msys2term の `-v` オプションで環境変数 `MSYSTEM` を設定して起動します。
MSYS2 の /etc/profile は `MSYSTEM` の値によって PATH などを設定し、各環境の shell になります。

| id           | メニュー            | MSYSTEM      |
|--------------|---------------------|--------------|
| `msys2`      | MSYS2 MSYS          | `MSYS`       |
| `mingw32`    | MSYS2 MinGW x32     | `MINGW32`    |
| `mingw64`    | MSYS2 MinGW x64     | `MINGW64`    |
| `ucrt64`     | MSYS2 UCRT64        | `UCRT64`     |
| `clang64`    | MSYS2 CLANG64       | `CLANG64`    |
| `clangarm64` | MSYS2 CLANGARM64    | `CLANGARM64` |

ttlaunch は msys-2.0.dll をロードできるよう、環境変数 PATH の先頭に MSYS2 の usr\bin を追加して
msys2term を起動します。
例えば MinGW x64 の起動は、次の bat と同等です。

```
set PATH=c:\msys64\usr\bin;%PATH%
set MSYSTEM=MINGW64
\path\to\msys2term.exe
```

MSYS2 の環境については次の URL を参照してください。

- https://www.msys2.org/docs/environments/
- https://www.msys2.org/wiki/Launchers/

## WSL

WSL は、cygterm または msys2term の `-s` オプションで wsl.exe を shell として実行して起動します。
cygterm, msys2term のどちらを使うかは、Cygwin, MSYS2, Git for Windows の順に見つかったものを使用します。

ディストリビューションは、レジストリ
HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Lxss
の各サブキーの DistributionName から取得します(docker-desktop* は除く)。
ディストリビューションが 1 つも無い場合、WSL の項目は表示しません。

- デフォルトのディストリビューション
  - メニューは `WSL (<デフォルトのディストリビューション名>)`、id は `wsl`
  - `wsl.exe` を引数なしで実行します
  - デフォルトのディストリビューションは Lxss の DefaultDistribution で、`wsl --set-default <ディストリビューション名>` で変更できます
- ディストリビューションを指定
  - メニューは `WSL: <ディストリビューション名>`、id は `wsl:<ディストリビューション名>`
  - `wsl.exe -d <ディストリビューション名>` を実行します

例えば Ubuntu-24.04 の起動は、次のコマンドと同等です。

```
cygterm -nols -s "C:/Windows/system32/wsl.exe -d 'Ubuntu-24.04'"
```

## CHERE_INVOKING

`-d` を指定したとき、Cygwin, MSYS2, Git Bash の shell には
cygterm または msys2term の `-v` オプションで環境変数 `CHERE_INVOKING=y` を設定します。

cygterm, msys2term も、`-d` が指定されていて、ログインシェルとして起動するとき
(設定ファイルの `LOGIN_SHELL = Yes`)は、`CHERE_INVOKING=y` を設定します。
通常の設定では ttlaunch の `-v` オプションと重複しますが、
ttlaunch の指定は `LOGIN_SHELL` の設定にかかわらず有効です。

環境変数 `CHERE_INVOKING` は Cygwin の chere パッケージ
(エクスプローラーのコンテキストメニューに "Bash Prompt Here" などを追加するパッケージ)
由来の環境変数です。

Cygwin, MSYS2 の /etc/profile はログインシェルとして起動したときに
ホームディレクトリへ移動しますが、
環境変数 `CHERE_INVOKING` が設定されているときは移動しません。
これにより、`-d` で指定したフォルダで shell が開始します。

WSL と Windows の shell(cmd, PowerShell) には設定しません。

## エクスプローラーのコンテキストメニュー

エクスプローラーのコンテキストメニュー(右クリックで表示されるメニュー)に
"Open in Tera Term" を登録して、フォルダを指定して shell を起動できます。
登録は ttlaunch の設定ダイアログ(下記)で行います。インストーラーは登録しません。
HKEY_CURRENT_USER に登録するので、管理者権限は不要です。

### 登録されるレジストリ

Popup menu では次のキーを登録します。

```
[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch]
@="&Open in Tera Term"
"Icon"="\"C:\\path\\to\\ttlaunch.exe\""

[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch\command]
@="\"C:\\path\\to\\ttlaunch.exe\" -d \"%V\""
```

Items では、shell ごとに次のキーを登録します(項目は環境によって変わります)。

```
[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch_01_cygwin]
@="Tera Term: &Cygwin"
"Icon"="C:\\cygwin64\\Cygwin.ico"

[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch_01_cygwin\command]
@="\"C:\\path\\to\\ttlaunch.exe\" -e cygwin -d \"%V\""
```

Submenu では次のように登録します。

```
[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch]
"MUIVerb"="&Open in Tera Term"
"Icon"="\"C:\\path\\to\\ttlaunch.exe\""
"SubCommands"=""

[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch\shell\01_cygwin]
"MUIVerb"="&Cygwin"
"Icon"="C:\\cygwin64\\Cygwin.ico"

[HKEY_CURRENT_USER\Software\Classes\Directory\Background\shell\ttlaunch\shell\01_cygwin\command]
@="\"C:\\path\\to\\ttlaunch.exe\" -e cygwin -d \"%V\""
```

- `SubCommands` が空のキーは、その `shell` サブキーの項目をサブメニューとして表示します(Windows 7 以降)
- 親キーに既定値(`@=`)があるとサブメニューの項目が表示されないため、Submenu では `MUIVerb` を使用します。
  このため登録時は既存のキーを削除してから書き直します(reg ファイルでは `[-キー]` で削除します)
- 項目はキー名のアルファベット順に並ぶため、`01_`, `02_` のような番号を付けています。
  Items では他のアプリの項目と混ざって並ぶので、`ttlaunch_` の接頭辞でまとめています
- `CommandFlags` の `0x20` は、その項目の前に区切り線を入れる指定です(サブメニューのみ)
- フォルダ自体の右クリック(`Folder\shell`)では `%V` の代わりに `%L` を使用します。
  開いているフォルダの背景(`Directory\Background\shell`)、ライブラリ(`LibraryFolder\Background\shell`)にも同じ内容を登録します
- Windows 11 では、クラシックなコンテキストメニュー("その他のオプションを確認")に表示されます

## 環境の検索

Cygwin は TERATERM.INI の CygwinDirectory とレジストリ、
Git for Windows はレジストリで見つからなかった場合、
固定ドライブ上の \cygwin64, \msys64, \Program Files\Git などを探します。

固定ドライブは `C:` から `Z:` で、GetDriveTypeW() が DRIVE_FIXED を返すドライブです。

ネットワークドライブやリムーバブルドライブにアクセスして、
動作が遅くなったり、利用者が不快に感じたりしないようにするため探すドライブを絞っています。
