
# libs directory

- This directory for storing external libraries to build Tera Term.
- Source, library and executable file are stored for each compiler.
- Library is generated only once in advance.

# Preparing

## Visual Studio

- cmake
  - It is OK if PATH is passed.
  - Do not use Cygwin's cmake(Not supporting for Visual Studio).
  - Use cmake 3.11.4 when Visual Studio 2005 is used.
- perl
  - It is necessary for converting character code of document and code of carriage return.
  - compiling OpenSSL (Tera Term 5 does not link OpenSSL because it switched to LibreSSL)
  - ActivePerl 5.8 or later, also cygwin perl.
  - It will be searched automatically if PATH is not passed.

## MinGW common (experimental)

- Can be built with MinGW on Cygwin,MSYS2,linux(wsl).
- The cmake,make,(MinGW)gcc,(clang) and perl that work in each environment are required.

# How to build

You need to use Internet service because some archives are automatically downloaded.

## Case of Visual Studio

### By using batch file

Execute libs/buildall_cmake.bat, and select Visual Studio.

    1. Visual Studio 17 2022 Win32
    2. Visual Studio 17 2022 x64
    3. Visual Studio 17 2022 arm64
    4. Visual Studio 16 2019 Win32
    5. Visual Studio 16 2019 x64
    6. Visual Studio 15 2017
    7. Visual Studio 14 2015
    8. Visual Studio 12 2013
    9. Visual Studio 11 2012
    a. Visual Studio 10 2010
    b. Visual Studio 9 2008
    select no

### By using cmake

Case of Visual Studio 2022 x86

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=win32 -P buildall.cmake

Case of Visual Studio 2022 x64

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=x64 -P buildall.cmake`

Case of Visual Studio 2022 arm64

    cmake -DCMAKE_GENERATOR="Visual Studio 17 2022" -DARCHITECTURE=arm64 -P buildall.cmake`

## MinGW common

Using cmake in each environment.

    cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake

# Regarding each directory

## Library directory generated

- Library `*.h` and `*.lib` are created in the following:
    - `cJSON`
    - `oniguruma_{compiler}`
    - `libressl_{compiler}`
    - `SFMT_{compiler}`
    - `zlib_{compiler}`

## Downloaded archive directory

- Downloaded archives are stored.
- Downloading automatically.
- Reuse these archives downloaded already.
- Can be removed if these archives do not need after building.

## Build directory

- Building under `build/oniguruma/{compiler}/`.
- Remove it in advance if rebuilding.
- Can be removed if this do not need after building.
