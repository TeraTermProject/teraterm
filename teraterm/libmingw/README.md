build with MinGW
================

## [Mingw-w64]

- [MinGW(Wikipedia)](https://ja.wikipedia.org/wiki/MinGW)
- [Mingw-w64]はMinGWのフォーク、現在はMingw-w64を使用
- コンパイラ(GCC,Clang)、exeファイルを作成するためのツール、ライブラリ等のセット
- プロジェクトによっていくつかの選択がある
  - GCC(clang)のバージョン
  - ライブラリのバージョン
  - ターゲット(i686(32bit)/x86_64(64bit))
  - Thread model (win32/posix)
  - 例外 (sjlj/dwarf)
- 比較的新しいOSでTera Termを動作させるならどれでもokと思われる

## printf()系の動作を Visual Studio と同じにする

- MinGW は C99,C11 などの仕様に準拠しようとしている
- 古い Visual Studio は仕様策定前の実装
  - "%zd" など新しいフォーマット指定子
    - 古い Visual Studio(msvcrt.dll) の printf()系ではサポートしていない
  - L"%s" と "%s" の動作の違い
    - msvcrtでは "%s" は char, L"%s" は wchar_t を引数とする
    - MinGWでは "%s", L"%s" とも char, "%ls" は wchar_t を引数とする
- 特に指定していないと MinGW の printf()系(stdio)を使用する
- このため Visual Studio と動作が異なってしまう
- `__USE_MINGW_ANSI_STDIO` マクロので切り替えることができる
  - 1 のとき mingw の stdio (mingwのデフォルト)
  - 0 のとき msvcrt
- 参考URL
  - [printf の %lf について](https://ja.stackoverflow.com/questions/34013/printf-%E3%81%AE-lf-%E3%81%AB%E3%81%A4%E3%81%84%E3%81%A6)
- Visual Studio 2015 以降
  - `_CRT_STDIO_ISO_WIDE_SPECIFIERS` を使用すると C99 に準拠するらしい

## MinGW([Mingw-w64]) で古いOSをサポートする

ポイント

- ランタイム(msvcrt.dll)の違いの吸収
- dllのロード(loadlibrary())
- pthreadを使用しない
- libgcc の gthr-win32 をリンクしない(GCC 13 以降)

## ランタイム(msvcrt.dll)の違いの吸収

- MinGW は C runtime として msvcrt.dll (Visual Studioのランタイムdll)を使用する
- WindowsのバージョンによってインストールされているランタイムDLL のバージョンが異なる
- 使用可能な関数が異なっている
  - [msvcrt-xref.pdf](https://sourceforge.net/projects/mingw/files/MinGW/Base/mingwrt/msvcrt-xref/)

- CランタイムDLLのバージョンを吸収するためのwrapperを作成
  - msvcrt_wrapper.c を作成
- Windows System Libraries (WSL) を参照
  - https://sourceforge.net/p/mingw/news/

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
  - Windows Vista / Server 2008 以降からok、それ以前では問題がある
  - [スレッド固有記憶領域を持つ DLLを LoadLibrary すると異常動作する問題](http://seclan.dll.jp/dtdiary/2011/dt20110818.htm)
- Windowsではexe(PE32)内.tlsセクションを利用して、スレッドローカル変数の処理を行う
  - スレッドコールバックテーブル
- Tera Termではスレッドローカル変数を使用していない
- MinGWのCランタイムスタートアップの一部を置き換えてtlsセクションを生成しないようにした

## gthr-win32 をリンクしない (GCC 13 以降)

### gthr-win32 について

- gthr(gthreads) は libgcc / libstdc++ が内部で使うスレッド抽象化層
  - Thread model win32 では gthr-win32、posix では gthr-posix(winpthreads) が使われる
- libgcc.a, libstdc++.a は GCC のランタイムライブラリ
  - MinGW-w64(ヘッダ、CRT)とは別のもので、バージョンも別
  - GCC が生成するコードの実行に必要な処理(64bit 除算、スタックチェックなど)が入っている
  - Ubuntu の clang で MinGW 向けにビルドした場合も、GCC の libgcc.a, libstdc++.a が使われる
- GCC 13 で gthr-win32 が作り直され、新しい Win32 API を使用するようになった
  - libgcc.a の gthr-win32.o
    - TryEnterCriticalSection(), GetThreadId() など
  - libgcc.a の gthr-win32-cond.o (条件変数)
    - InitializeConditionVariable(), SleepConditionVariableCS() など (Vista以降)
- これらがリンクされると Windows 95 では exe/dll がロードできない

### gthr-win32 がリンクされる経路と対策

libstdc++ の次のオブジェクトが gthr-win32 を参照している

| 経路                                                      | リンクされるもの                           | 対策                       |
|-----------------------------------------------------------|--------------------------------------------|----------------------------|
| 関数内 static 変数のスレッドセーフな初期化                | guard.o → gthr-win32.o, gthr-win32-cond.o | `-fno-threadsafe-statics`  |
| throw / catch                                             | 例外処理一式 → eh_alloc.o → gthr-win32.o | `-fno-exceptions`          |
| operator new (std::bad_alloc を throw する)               | new_op.o → 例外処理一式                   | new_delete.cpp で置き換え  |
| std::vector など (std::__throw_length_error() などを呼ぶ) | functexcept.o → 例外処理一式              | functexcept.cpp で置き換え |

- コンパイルオプションは CMakeLists.txt で `SUPPORT_OLD_WINDOWS` のとき指定している
- new_delete.cpp, functexcept.cpp は両方必要
  - 片方だけでは、もう片方の経路から例外処理一式と gthr-win32.o がリンクされる

### 使用しないこと

上記の対策をしてもリンクされてしまうため、次のものは使用しない

- std::string, std::wstring
  - libstdc++.a 内の実装(string-inst.o)が例外ありでビルドされていて、
    `__cxa_begin_catch` などを参照する → 例外処理一式 → gthr-win32.o
  - ヘッダ側の関数の置き換えでは防げない
- std::thread, std::mutex, std::condition_variable など
- thread_local
  - TLS の問題もある(前述)
- 上記以外でも、ヘッダだけで完結せず libstdc++.a 内の実装を呼ぶものは同様の可能性がある

### 確認方法

- インポートしている API を確認する
  ```
  i686-w64-mingw32-objdump -p ttermpro.exe | grep -E "TryEnterCriticalSection|GetThreadId|ConditionVariable"
  ```
- リンクマップで、何が gthr-win32.o を引き込んでいるか確認する
  - `-Wl,-Map=xxx.map` でリンクし、"Archive member included to satisfy reference by file (symbol)" を見る
  - 1行目がリンクされたオブジェクト、2行目が参照元のオブジェクト(シンボル)
  ```
  .../libgcc.a(gthr-win32.o)
                                .../libstdc++.a(eh_alloc.o) (__gthr_win32_mutex_init_function)
  ```

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

```
$ cat /etc/os-release  | grep -i pretty
PRETTY_NAME="Ubuntu 26.04 LTS"
```

次のパッケージをインストールしておく
```
sudo apt-get update
sudo apt-get -y upgrade
sudo apt-get -y install cmake perl
sudo apt-get -y install mingw-w64-i686-dev g++-mingw-w64
sudo apt-get -y install fp-utils nasm
cd /path/to/teraterm
```

thread model win32版が動作するか次のコマンドなどでチェックしておく
```
i686-w64-mingw32-gcc -v
i686-w64-mingw32-g++ -v
update-alternatives --display i686-w64-mingw32-gcc
update-alternatives --display i686-w64-mingw32-g++
```

gcc を使う場合の例
```
cd [Tera Term source dir]
cd libs
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
cd ..
mkdir build_mingw32_gcc
cd build_mingw32_gcc
cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DSUPPORT_OLD_WINDOWS=ON -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
make -j 4
make -j 4 install
bash make_installer_cmake.sh
```

clang を使う場合
```
sudo apt-get install -y clang
mkdir build_mingw32_clang
cd build_mingw32_clang
cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DSUPPORT_OLD_WINDOWS=ON -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DUSE_CLANG=ON
make -j 4
make -j 4 install
bash make_installer_cmake.sh
```

[Mingw-w64]: https://www.mingw-w64.org/
