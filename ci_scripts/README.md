# CI用スクリプト

- AppVeyor用スクリプト
- ローカルでビルド
  - AppVeyor用スクリプトをローカルでテスト
  - これを使えば簡単にビルドすることができる

## Visual Studio 2005

- Visual Studio 2005 をインストールする
- 次のバッチファイルを実行する
  - build_local_appveyor_vs2005.bat

注
 Visual Studio 2005 関連のファイルは入手が難しいため
 新たにインストールするのは困難

## Visual Studio 2019

- Visual Studio 2019 をインストールする
- 次のバッチファイルを実行する
  - build_local_appveyor_vs2019.bat
  - build_local_appveyor_vs2019_x64.bat

## msys2

- msys2を使ったビルド
- インストーラーをダウンロード、インストールする
  - https://www.msys2.org/
- 次のどれか一つバッチファイルを実行する
  - ci_scripts\build_local_appveyor_mingw_x64_gcc.bat
  - ci_scripts\build_local_appveyor_mingw_clang.bat
  - ci_scripts\build_local_appveyor_mingw_gcc.bat
  - ci_scripts\build_local_appveyor_mingw_x64_clang.bat
