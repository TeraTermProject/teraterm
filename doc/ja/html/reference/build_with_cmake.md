# cmakeを使ったビルド

- [cmake](<https://cmake.org/>)を使用して
  ビルドすることができます(実験的な位置づけです)
- Visual Studio, NMake, MinGW でビルドできます

## cmakeのバージョン

- 3.11以上
- Visual Studio 2017,2019,2022,2026 インストーラーで、オプションを選べば cmake をインストールできます
- Visual Studio 2005(Expressも含む)のIDEをサポートしている最後のバージョンは 3.11.4 です

## ライブラリ

- teraterm が使用するライブラリをビルドして準備しておきます
  - ビルドに使用するツールに合わせたライブラリが必要です
- ビルド方法は [`build_library_with_cmake.html`](<build_library_with_cmake.html>) を参照してください
- ライブラリについては [`develop.html`](<develop.html>) を参照してください

## teratermのビルド

### Visual Studio

Visual Studio の IDE を使用する場合の例
(`-A win32` はx86指定。x64の場合は `-A x64`, ARM64の場合は `-A arm64`)

    mkdir build_vs2022
    cd build_vs2022
    cmake .. -G "Visual Studio 17 2022" -A Win32
    cmake --build . --config Release -j

インストーラを作成する

    cmake --build . --config Release --target Install
    make_installer_cmake.bat

### NMake (Visual Studio, very experimental)

vcvars32.bat を実行しておくなどして
nmakeが利用できる環境から次のように実行します。

    mkdir build_nmake
    cd build_nmake
    cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j

### MinGW (very experimental)

- MinGW を使用してバイナリを生成することができます
- 実験的位置づけです

msys2等をつかってMinGWが使える環境から次のように実行します。

    mkdir build_mingw_msys2_i686
    cd build_mingw_msys2_i686
    cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
    cmake --build . -j

cygwin,linux等では次のように実行します。

    mkdir build_mingw_cygwin_i686
    cd build_mingw_cygwin_i686
    cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
    cmake --build . -j
