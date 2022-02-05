# Tera Term が使用するフォルダ

|                  | インストール版(*1)         | ポータブル版                 |
|------------------|----------------------------|------------------------------|
| 設定ファイル     | `%APPDATA%\teraterm5`      | exeファイルのあるフォルダ    |
| 動作ログ、ダンプ | `%LOCALAPPDATA%\teraterm5` | `%USERPROFILE%\Documents\teraterm5` (*2) |

- *1 インストール先 (32bitOS+32bitEXEの場合) `%PROGRAMFILES%\teraterm5`
- *2 SHGetKnownFolderPath(FOLDERID_Documents) + "\teraterm5"

# ポータブル版について

- 環境(システム設定、個人設定)に左右されず使用することができる
  - 次のような用途を想定
    - USBメモリに入れて、そこから起動する
    - インストールせずに使用する (コピーするだけで使えるようになる)
- 実行する ttermpro.exe と同じフォルダに portable.ini ファイルを置くことでポータブル版として動作
- portable.ini の中身はなんでもよい(サイズ0でよい)

# インストール版の使用するフォルダ,ファイルについて

*注意*
現在 Tera Term 5 は Windows10,11 のみをターゲットとしている

## Windows 10, 11 のフォルダ例

- 設定ファイル
  - `%APPDATA%\teraterm5`
- 動作ログ、ダンプ
  - `%LOCALAPPDATA%\teraterm5`

## Windows Vista以降 の場合のフォルダ例

- 設定ファイル
  - `C:\Users\[usernane]\AppData\Roaming\teraterm5`
- 動作ログ、ダンプ
  - `C:\Users\[usernane]\AppData\Local\teraterm5`

## Windows 2000, XP の場合のフォルダ例

- 設定ファイル
  - `C:\Documents and Settings\[usernane]\AppData\Roaming\teraterm5`
- 動作ログ、ダンプ(未定義)
  - `C:\Documents and Settings\[usernane]\Application Data\teraterm5`

## Windows 2000より以前の場合のフォルダ例

- 設定ファイル
  - `C:\My Documents\teraterm5`
- 動作ログ、ダンプ
  - `C:\My Documents\teraterm5`

参考

- [https://torutk.hatenablog.jp/entry/20110604/p1](https://torutk.hatenablog.jp/entry/20110604/p1)

## ファイルを指定した場合

コマンドラインでファイルをフルパスで指定した場合は、それを優先して使用する

- TERATERM.INI
- KEYBOARD.CNF
- broadcast.log


## 設定ファイルがない場合

次のように動作する

- 設定ファイルを置くフォルダを作成する
- ttermpro.exeのフォルダにある設定ファイルを
- 設定ファイルを置くフォルダにコピーする
