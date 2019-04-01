# How to build by using cmake

- You can build Tera Term by using [cmake](<https://cmake.org/>)(EXPERIMENTAL).

## cmake version

- The final version of cmake supporting for Visual Studio 2005 is 3.11.4.
- The ttpmacro.exe can not be built with Visual Studio 2005 Express.
- This no restriction by using Visual Studio any version other than 2005(including Express).
- When the cmake option is selected on Visual Studio 2017 installer, the cmake can be installed.

## MinGW (very experimental)

- You can create the binary file by using MinGW.
- EXPERIMENTAL
- The ttpmacro.exe can not be built with MinGW.

## How to build library

- You can prepare libraries used by Tera Term.
- Refer to `lib/build_library_with_cmake.md`. 
- Refet to `develop.txt`.

## How to build Tera Term

Please execute below commands on the top of source tree.

    mkdir build_vs2005
    cd build_vs2005
    ..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe .. -G "Visual Studio 8 2005"
    ..\libs\cmake-3.11.4-win32-x86\bin\cmake.exe --build . --config release

- Change a string after `-G` option according to Visual Studio version.
- The sln file is created, so the file can be opened with Visual Studio. 
- If the cmake is included in path, you need not write the full path of cmake.
- When Visual Studio is launched by sln file and is building, the Visual Studio can re-generate the project file after detecting changes to CMakeLists.txt. So, the sln file is manually created only once.

## How to build Tera Term(MinGW)

Please execute below commands on the top of source tree by using the cmake available on MinGW.

    mkdir build_mingw_test
    cd build_mingw_test
    cmake .. -G "Unix Makefiles"
    make -j4
