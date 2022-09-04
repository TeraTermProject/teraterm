# ttreg

カレントディレクトリの ttermpro.exe のショートカットの作成とエクスプローラー
のコンテキストメニューを追加するための reg ファイルを作成するツール。

インストーラーを使用しなくてもポータブル版や開発中フォルダの Tera Term
を便利に使用することができます。

Tera Term から cygwin, msys2(MSYS, MinGW x32, MinGW x64), git bash を使
用するためのショートカット、エクスプローラーのコンテキストメニューから
フォルダを指定しての起動が簡単にできます

## 使用方法

ttreg.exe を実行すると次のファイルが出力されます。

- shortcuts/ にショートカット
- explorer_context.reg

実行例

    >./ttreg.exe
    cygwin found
    msys2 found
    git bash found
    mkdir 'C:\tmp\snapshot\shortcuts'
    'explorer_context.reg' write
    >ls *.reg shortcuts/
    explorer_context.reg*
    
    shortcuts/:
    'Tera Term + cmd.lnk'*              'Tera Term + msys2_MinGW_x64.lnk'*
    'Tera Term + cygwin.lnk'*           'Tera Term + msys2_MSYS.lnk'*
    'Tera Term + git_bash.lnk'*         'Tera Term.lnk'*
    'Tera Term + msys2_MinGW_x32.lnk'*

## ショートカット

ショートカットを適宜コピーして使用することができます。

## explorer context

出力された explorer_context.reg をダブルクリックすると
explorerの右クリックにメニューが追加されます。

次のレジストリに登録されます。
HKEY_CURRENT_USER\SOFTWARE\Classes\Folder\shell
不要になったらregeditで削除してください。

## 制限

cygterm.exe 経由で cygwin を利用することは以前から行われていましたが、
msys2term.exe 経由で msys2, git bash を利用することは行われていないので
何らかの問題が出る可能性があります。

cmd, powershell, wsl の利用はある程度動作していますが、
何らかの不具合が出る可能性があります。
