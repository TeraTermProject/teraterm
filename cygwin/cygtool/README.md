# cygtool.dll

- インストーラー inno setup のスクリプトで使用
- 32bit dll でなければならない

## 機能

次のエントリを export している

- FindCygwinPath
  - Cygwinがインストールされているフォルダを返す
- PortableExecutableMachine
  - DLL(cygwin1.dll)が32/64bit版を返す
- CygwinVersion
  - DLL(cygwin1.dll用)のバージョンを返す

