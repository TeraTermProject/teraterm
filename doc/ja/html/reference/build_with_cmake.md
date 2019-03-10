# cmakeを使ったビルド

- 実験的に cmake を使用してビルドすることができます
- cmake https://cmake.org/

## cmakeのバージョン

- Visual Studio 2005 をサポートしている cmake の最後のバージョンは 3.11.4 です
- 2005 以外の Visual Studio を使用する場合は特に制限はありません
- Visual Studio 2017 インストーラーで、オプションを選べば cmake をインストールできます

## ライブラリのビルド

- teraterm が使用するライブラリをビルドして準備しておきます
- `lib/build_library_with_cmake.md` を参照してください
- ライブラリは `develop.txt` を参照してください

## teratermのビルド

- ソースツリーのトップから、次のようにコマンドを実行します  
    ```
    mkdir build_vs2005
    cd build_vs2005
    "c:\Program Files (x86)\CMake-3.11.4\bin\cmake.exe" .. -G "Visual Studio 8 2005"
    ```
    - パスが通っていれば cmake はフルパスで書く必要はありません
    - `-G` オプションの後ろは使用する Visual Studio のバージョンに合わせて調整します
- sln ファイルが生成されるので Visual Studio で開くことができます
- cmakeを使ってビルドする場合は次のコマンドを実行します  
    ```
    cmake --build . --config release
    ```
