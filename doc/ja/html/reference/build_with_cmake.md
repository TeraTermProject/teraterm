# cmakeを使ったビルド

- [cmake](<https://cmake.org/>)を使用して
  ビルドすることができます(実験的な位置づけです)

## cmakeのバージョン

- Visual Studio 2005 をサポートしている cmake の最後のバージョンは 3.11.4 です
- Visual Studio 2005 Express では ttpmacro.exe をビルドすることができません
- Visual Studio 2005 (Expressも含む)以外を使用する場合は特に制限はありません
- Visual Studio 2017 インストーラーで、オプションを選べば cmake をインストールできます

## MinGW (very experimental)

- MinGW を使用してバイナリを生成することができます
- 実験的位置づけです
- MinGW では ttpmacro.exe をビルドすることができません

## ライブラリのビルド

- teraterm が使用するライブラリをビルドして準備しておきます
- `lib/build_library_with_cmake.md` を参照してください
- ライブラリは `develop.txt` を参照してください

## teratermのビルド

ソースツリーのトップから、次のようにコマンドを実行します。

    mkdir build_vs2005
    cd build_vs2005
    ..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe .. -G "Visual Studio 8 2005"
    ..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe --build . --config release

- `-G` オプションの後ろは使用する Visual Studio のバージョンに合わせて調整します
- sln ファイルが生成されるので Visual Studio で開くことができます
- パスが通っていれば cmake はフルパスで書く必要はありません
- 生成された sln ファイルから起動したVisual Studioでビルドすると
  CMakeLists.txt を変更を検出してプロジェクトファイルの再生成を自動で行うので
  sln ファイルの生成を手動で行うのは最初の1回だけです

## teratermのビルド(MinGW)

MinGWので使用できるcmakeを使って、
ソースツリーのトップから、次のようにコマンドを実行します。

    mkdir build_mingw_test
    cd build_mingw_test
    cmake .. -G "Unix Makefiles"
    make -j4
