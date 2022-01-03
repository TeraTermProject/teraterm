AppVeyor Linux build
====================

linux上の MinGW を利用したビルド

## プロジェクト作成 / 設定

- appveyor.yml の代わりに appveyor_linux.yml を使用する

# ローカルでのテスト

## ビルド準備/WSL

- ストアで debian をインストールする
  - 21-05-02時点で debian 10(buster)
- debian を起動
- 次のコマンドを実行

```
sudo apt-get update
sudo apt-get -y upgrade
sudo apt-get -y install cmake perl subversion
sudo apt-get -y install g++-mingw-w64
sudo apt-get -y install fp-utils
cd /path/to/teraterm
```

## ビルド準備/debian

- WSLと同じ
- 次のディストリビューション/バージョンに含まれているmingwでのビルドは確認した

debian buster

```
$ cat /etc/debian_version
10.9
```

debian bullseye/sid

```
$ cat /etc/debian_version
bullseye/sid
```

## ビルド

gcc 32bit
- `cmake -P ci_scripts/build_local_appveyor_mingw.cmake`

gcc 64bit
- `cmake -DCOMPILER_64BIT=ON -P ci_scripts/build_local_appveyor_mingw.cmake`

msys64
- [build_local_appveyor_mingw_cmake.bat](build_local_appveyor_mingw_cmake.bat)参照
