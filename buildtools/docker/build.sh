git clone --branch "${BRANCH:-main}" --depth 1 "${URL:-https://github.com/TeraTermProject/teraterm.git}" teraterm
cd teraterm
cd libs
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
rm -rf build
cd ..
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DARCHITECTURE=i686 -DSUPPORT_OLD_WINDOWS=on
make -j $(nproc)
make -j install
bash make_installer_cmake.sh
cp *.zip /mnt/
