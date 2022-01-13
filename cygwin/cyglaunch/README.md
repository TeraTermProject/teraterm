# cyglaunch

cygterm を起動するプログラム

## コマンドラインオプション

cyglaunch で解釈されない引数は cygterm(又は msys2term)へ渡される

-msys2
    cygterm ではなく msys2term を起動する
    次のオプションを付けることで msys2の環境を設定できる

    -v MSYSTEM=MSYS
        msys
    -v MSYSTEM=MINGW32
        mingw32
    -v MSYSTEM=MINGW64
        mingw64

    msys2の起動については次のURLを参照
    https://www.msys2.org/wiki/Launchers/

## from explorer

レジストリに設定することで、エクスプローラーのコンテキストメニューからターミナルをオープンできる

cygterm_here_reg_sample.txt 参照

## cyglaunchの動作

cygterm.exe の起動と同等なbat

```
set PATH=c:\cygwin64\bin;%PATH%
\path\to\cygterm.exe
```

msys2term.exe の起動と同等なbat

```
set PATH=c:\msys64\bin;%PATH%
\path\to\msys2term.exe
```

## TODO

cyglaunchを拡張して次のbatと同等に起動させれば、Tera Termから使用できる

- cyglaunch_cmd.bat
  - cmd.exe を Tera Term 内で動作させる
  - Microsoft Windows [Version 10.0.19043.1348] で動作確認
- cyglaunch_powershell.bat
  - powershell を Tera Term 内で動作させる
- cyglaunch_wsl.bat
  - wsl を Tera Term 内で動作させる

## 歴史

- cyglaunch は通常のexeファイル
- Visual Studio で普通にビルドできる
- cygwin上で動作する必要はない
- 従来は cygterm フォルダに存在し MinGW でビルドされていた
