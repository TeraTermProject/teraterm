# 外部ライブラリ

## 作成するライブラリ

- zlib
- pnglib
- jpeglib

## ビルド方法

    cmake -P extlib.cmake

### vs2019 x64

例

    rmdir /s /q build
    cmake -DGENERATE_OPTION="-G;Visual Studio 16 2019;-Ax64" -DINSTALL_PREFIX_ADD="vs2019_x64/" -P extlib.cmake

### vs2019 win32

例

    rmdir /s /q build
    cmake -DGENERATE_OPTION="-G;Visual Studio 16 2019;-AWin32" -DINSTALL_PREFIX_ADD="vs2019_win32/" -P extlib.cmake


## 作業フォルダ

ビルド後、次のフォルダは削除しても大丈夫です。

- build
- download
