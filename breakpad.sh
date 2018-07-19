#!/bin/sh
set -ex

if [ ! -d "breakpad" ]; then
  mkdir breakpad
fi

cd breakpad

if [ ! -d "depot_tools" ]; then
  git clone --depth=1 --branch=master https://chromium.googlesource.com/chromium/tools/depot_tools.git depot_tools
fi

if [ ! -d "src" ]; then
  ./depot_tools/fetch --nohooks breakpad
else
  ./depot_tools/gclient sync --nohooks
fi

if [ ! -d "build" ]; then
  mkdir build
fi

cd build

../src/configure --enable-m32

make src/tools/linux/dump_syms/dump_syms
make src/client/linux/libbreakpad_client.a
make src/libbreakpad.a src/third_party/libdisasm/libdisasm.a
