clone=true
#clone=false
if $clone; then
  git clone --branch "${BRANCH:-main}" --depth 1 "${URL:-https://github.com/TeraTermProject/teraterm.git}" teraterm
else
  git config --global --add safe.directory /workspaces/teraterm/teraterm_host
  (cd teraterm_host/ && git archive HEAD -o ../current.tar)
  (mkdir -p teraterm && cd teraterm && tar xf ../current.tar)
  cp -r teraterm_host/libs/download/* teraterm/libs/download
fi
cd teraterm
cd libs
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
rm -rf build
cd ..
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DARCHITECTURE=i686 -DSUPPORT_OLD_WINDOWS=on -DCMAKE_BUILD_TYPE=Release
make -j $(nproc)
make -j install
bash make_installer_cmake.sh
cp *.zip /mnt/
