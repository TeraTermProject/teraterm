clone=true
#clone=false
if $clone; then
  git clone --branch main --depth 1 https://github.com/TeraTermProject/teraterm.git
  cd teraterm
else
  git config --global --add safe.directory /mnt
  (cd /mnt; git archive HEAD -o /current.tar)
  mkdir teraterm
  cd teraterm
  tar xf /current.tar
fi
cd libs
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=32 -P buildall.cmake
rm -rf build
cd ..
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DUSE_GCC_32=on -DSUPPORT_OLD_WINDOWS=on -DENABLE_GDIPLUS=off
make -j
make -j install
make zip
cp *.zip /mnt/buildtools/docker/
