# cmakeを使ったビルド

- cmake を使用してビルドすることができます(実験的な位置づけです)
	- https://cmake.org/

## cmakeのバージョン

- Visual Studio 2005 をサポートしている cmake の最後のバージョンは 3.11.4 です
- Visual Studio 2005 以外を使用する場合は特に制限はありません
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
	..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe .. -G "Visual Studio 8 2005"
    ```
    - `-G` オプションの後ろは使用する Visual Studio のバージョンに合わせて調整します
- sln ファイルが生成されるので Visual Studio で開くことができます
- cmakeを使ってビルドする場合は次のコマンドを実行します  
    ```
	..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe --build . --config release
    ```
- パスが通っていれば cmake はフルパスで書く必要はありません
- 生成された sln ファイルから起動したVisual Studioでビルドすると
  CMakeLists.txt を変更を検出してプロジェクトファイルの再生成を自動で行うので
  sln ファイルの生成を手動で行うのは最初の1回だけです
