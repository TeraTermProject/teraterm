

## MinGW で古いOS(Windows95)をサポートする

ポイント

- ランタイム(msvcrt.dll)の違いの吸収
- dllのロード(loadlibrary())
- pthreadを使用しない

## ランタイム(msvcrt.dll)の違いの吸収

- MinGW は C runtime として msvcrt.dll (Visual Studioのランタイムdll)を使用する
- WindowsのバージョンによってインストールされているランタイムDLL のバージョンが異なる
- 使用可能な関数が異なっている
  - [msvcrt-xref.pdf](https://sourceforge.net/projects/mingw/files/MinGW/Base/mingwrt/msvcrt-xref/)

- CランタイムDLLのバージョンを吸収するためのwrapperを作成
  - msvcrt_wrapper.c を作成

### 未解決シンボルの追加について

vsnprintf_s() 場合の例

- msvcrt-os.a を nm で調べる
- 1つのオブジェクトファイルに次の2つのシンボルがあることがわかる
  - __imp__vsnprintf_s
  - _vsnprintf_s
- `__declspec(dllimport)` 属性の関数の場合
  - 関数へのポインタが `__imp_<関数名>` に収納される
- 2つのシンボルを解決できるようファイルに追加


```
libmsvcrt-oss01204.o:
00000000 b .bss
00000000 d .data
00000000 i .idata$4
00000000 i .idata$5
00000000 i .idata$6
00000000 i .idata$7
00000000 t .text
         U __head_lib32_libmsvcrt_os_a
00000000 I __imp__vsnprintf_s
00000000 T _vsnprintf_s
```

## dllをロードできるようにする

- プラグイン(dll)がロードできなくなる
- 原因は TLS(Thread Local Storage,スレッド毎の記憶領域)
- TLSを使用しないようにする
- MinGWのCランタイムスタートアップの一部を置き換える
  - tlssup.c

### TLSについて

- OSにはTLS(Thread Local Storage,スレッド毎の記憶領域)サポートが有る
- 新しいC,C++の仕様ではスレッドローカル変数が使用できる
  - C++11 では thread_local
  - C11 では _Thread_local
- Visual Studio では __declspec(thread) で使用できる
  - GCC では __thread
- LoadLibrary()で使用するdll(Tera Termの場合、プラグイン)では利用で問題があった
  - exe と同時にロードするdllはok
  - Windows 7以降からok
  - [スレッド固有記憶領域を持つ DLLを LoadLibrary すると異常動作する問題](http://seclan.dll.jp/dtdiary/2011/dt20110818.htm)
- Windowsではexe(PE32)内.tlsセクションを利用して、スレッドローカル変数の処理を行う
  - スレッドコールバックテーブル
- Tera Termではスレッドローカル変数を使用していない
- MinGWのCランタイムスタートアップの一部を置き換えてtlsセクションを生成しないようにした

## Windows 95向けビルドで使える MinGW

Windows 95で動作させるため、
Thread model が win32となっていることを確認する。
posix (pthread)を使用すると95に実装されていない Win32 APIを使用する
バイナリが生成される。

NGな例 (msys2)
```
$ gcc -v
Using built-in specs.
COLLECT_GCC=C:\msys64\mingw32\bin\gcc.exe
COLLECT_LTO_WRAPPER=C:/msys64/mingw32/bin/../lib/gcc/i686-w64-mingw32/10.2.0/lto-wrapper.exe
Target: i686-w64-mingw32
Configured with: ../gcc- ....
:
:
Thread model: posix
Supported LTO compression algorithms: zlib zstd
gcc version 10.2.0 (Rev5, Built by MSYS2 project)
```
cygwinも同様にNG

OKな例 (debian gcc-mingw-w64-i686-win32 package)
```
$ i686-w64-mingw32-gcc-win32 -v
Using built-in specs.
COLLECT_GCC=i686-w64-mingw32-gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/i686-w64-mingw32/10-win32/lto-wrapper
Target: i686-w64-mingw32
Configured with: ../../src/configure --build=x86_64-lin....
:
:
Thread model: win32
Supported LTO compression algorithms: zlib
gcc version 10-win32 20200525 (GCC)
```

OKな例 (MinGW-W64)
```
>gcc -v
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=C:/Program\ Files\ (x86)/mingw-w64/i686-8.1.0-win32-sjlj-rt_v6-rev0/mingw32/bin/../libexec/gcc/i686-w64-mingw32/8.1.0/lto-wrapper.exe
Target: i686-w64-mingw32
Configured with: ../../../src/gcc-8.1.0/configure --host=i68...
:
:
Thread model: win32
gcc version 8.1.0 (i686-win32-sjlj-rev0, Built by MinGW-W64 project)
```

### ビルド例 (debian gcc-mingw-w64-i686-win32 package)

Tera Term を Windows 95 で動作させるためのビルド

次のパッケージをインストールしておく
```
sudo apt-get install gcc-mingw-w64-i686-win32 g++-mingw-w64-i686-win32
```

thread model win32版が動作するか次のコマンドなどでチェックしておく
```
i686-w64-mingw32-gcc -v
i686-w64-mingw32-g++ -v
update-alternatives --display i686-w64-mingw32-gcc
update-alternatives --display i686-w64-mingw32-g++
```

ビルド例
```
cd [Tera Term source dir]
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DSUPPORT_OLD_WINDOWS=ON -DCMAKE_BUILD_TYPE=Release
make -j
make -j install
make -j zip
```

### ビルド例 (MinGW-W64)

create_locale()がうまく解決できないため今のところ使えない

- [MinGW-W64](http://mingw-w64.org/doku.php/download/mingw-builds)
- Version 8.1.0
- Architecture i686
- Thread win32
- Exception sjlj
- Build revision 0

例

```
cd [Tera Term source dir]
mkdir build_mingw-w64
cd build_mingw-w64
cmake .. -G "MinGW Makefiles"
mingw32-make -j
mingw32-make -j install
```

