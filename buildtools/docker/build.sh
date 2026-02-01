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
cmake -DCMAKE_GENERATOR="Unix Makefiles" -DARCHITECTURE=i686 -P buildall.cmake
rm -rf build
cd ..
mkdir build
cd build
cmake .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=../mingw.toolchain.cmake -DARCHITECURE=i686 -DSUPPORT_OLD_WINDOWS=on -DENABLE_GDIPLUS=off
make -j $(nproc)
make -j install
bash make_installer_cmake.sh
cp *.zip /mnt/buildtools/docker/
