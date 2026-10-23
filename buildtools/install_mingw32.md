# MinGW32

- 古い Windows 向けビルド(SUPPORT_OLD_WINDOWS=ON)用の MinGW32(gcc) を buildtools/ に展開します
- `install_mingw32.bat` (`cmake -P install_mingw32.cmake`) で展開します
  - buildtools/mingw32/ に展開されます

## MinGW-Builds

- https://github.com/niXman/mingw-builds-binaries/releases
- i686-16.2.0-release-win32-dwarf-msvcrt-rt_v14-rev1.7z を使用
  - i686 (32bit)
  - thread model win32
    - posix は winpthread を使用するため Windows 95 で動作しない
  - msvcrt
    - ucrt は Windows 95 で動作しない
- gcc, g++, windres, mingw32-make が含まれています

## Tera Term のビルド

このフォルダのbuild_mingw.batを実行してください。
```
build_mingw.bat
```

## 使用している Windows の API の確認

```
objdump -p ttermpro.exe
```

- GCC 13 以降の libgcc(gthr-win32) は Vista 以降の API を使用します
- SUPPORT_OLD_WINDOWS=ON のとき C++ の例外処理をリンクしないようにしています
  - teraterm/libmingw/
  - libcompatwin/
