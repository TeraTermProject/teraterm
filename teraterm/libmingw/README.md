build with MinGW
================

## MinGW

- [MinGW(Wikipedia)](https://ja.wikipedia.org/wiki/MinGW)
- いくつかの派生プロジェクトが存在する
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
