# How to build by using cmake

- You can build Tera Term by using [cmake](<https://cmake.org/>)(EXPERIMENTAL).
- You can bulid using Visual Studio, NMake, MinGW

## cmake version

- 3.11.4+
- When the cmake option is selected on Visual Studio 2017,2019,2022 installer, the cmake can be installed.
- The final version of cmake supporting for Visual Studio 2005 is 3.11.4.

## How to build library

- You can prepare libraries used by Tera Term.
- Refer to `lib/build_library_with_cmake.md`.
- Refet to `develop.txt`.

## How to build Tera Term

### Visual Studio

Please execute below commands on the top of source tree.
Use Visual Studio IDE
```
mkdir build_vs2022
cd build_vs2022
cmake .. -G "Visual Stuido 17 2022" -A Win32
cmake --build . --config Release -j
```

### NMake (Visual Studio, very experimental)

Execute vcvars32.bat,etc to prepare an environment where nmake, cl are avaiable,
Run next commands.
```
mkdir build_nmake
cd build_nmake
cmake .. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

### MinGW (very experimental)

- You can create the binary file by using MinGW.
- EXPERIMENTAL

Please execute below commands on the top of source tree by using the cmake available on MinGW.
```
mkdir build_mingw
cd build_mingw
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```

When cygwin, linux
```
mkdir build_mingw_cygwin
cd build_mingw_cygwin
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build . -j
```
