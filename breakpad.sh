#!/bin/sh
set -ex

if [ ! -d "breakpad" ]; then
  mkdir breakpad
fi

cd breakpad

if [ ! -d "depot_tools" ]; then
  git clone --depth=1 --branch=main https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
fi

if [ ! -d "src" ]; then
  PYTHONDONTWRITEBYTECODE=1 python3 ./depot_tools/fetch.py --nohooks breakpad
fi

# update breakpad to latest no matter what
git -C src fetch
git -C src reset --hard origin/main # <- you want main, not master!!!
PYTHONDONTWRITEBYTECODE=1 python3 ./depot_tools/gclient.py sync --nohooks


cd src
git config user.name patches || true
git config user.email patches@localhost || true
# explode if we can't apply patches
git am -3 --keep-cr ../../patches/*.patch || exit 1
cd ..

if [ ! -d "build" ]; then
  mkdir build
fi

cd build
env ac_cv_header_a_out_h=yes CXXFLAGS=-m32 CFLAGS=-m32 CPPFLAGS=-m32 ../src/configure --enable-m32 CFLAGS="-Wno-error=deprecated" CXXFLAGS="-Wno-error=deprecated -g -O2 -D_GLIBCXX_USE_CXX11_ABI=0" CPPFLAGS=-m32

make src/tools/linux/dump_syms/dump_syms
make src/client/linux/libbreakpad_client.a
make src/libbreakpad.a src/third_party/libdisasm/libdisasm.a
