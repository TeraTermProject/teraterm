# How to build by using cmake

- You can build Tera Term by using [cmake](<https://cmake.org/>)(EXPERIMENTAL).
- You can build using Visual Studio, NMake, MinGW

## cmake version

- 3.11.4+
- When the cmake option is selected on Visual Studio 2017,2019,2022 installer, the cmake can be installed.
- The final version of cmake supporting for Visual Studio 2005 is 3.11.4.

## How to build library

- You can prepare libraries used by Tera Term.
- Refer to [`build_library_with_cmake.html`](<build_library_with_cmake.html>).
- Refet to [`develop.html`](<develop.html>).

## How to build Tera Term

### Visual Studio

Please execute below commands on the top of source tree.
Use Visual Studio IDE
(`-A win32` specifies x86; `-A x64` for x64, `-A arm64` for ARM64)

    mkdir build_vs2022
    cd build_vs2022
    cmake .. -G "Visual Studio 17 2022" -A Win32
    cmake --build . --config Release -j

To build installer

    cmake --build . --config Release --target Install
    make_installer_cmake.bat

### NMake (Visual Studio, very experimental)

Execute vcvars32.bat,etc to prepare an environment where nmake, cl are available,
Run next commands.

    mkdir build_nmake
    cd build_nmake
    cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j

### MinGW (very experimental)

- You can create the binary file by using MinGW.
- EXPERIMENTAL

Please execute below commands on the top of source tree by using the cmake available on MinGW.

    mkdir build_mingw_msys2_i686
    cd build_mingw_msys2_i686
    cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
    cmake --build . -j

When cygwin, linux

    mkdir build_mingw_cygwin_i686
    cd build_mingw_cygwin_i686
    cmake .. -G "Unix Makefiles" -DARCHITECTURE=i686 -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake
    cmake --build . -j
