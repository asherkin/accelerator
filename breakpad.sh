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

find /usr/include/ -type f -iname 'a.out.h'

cat config.log

make src/tools/linux/dump_syms/dump_syms
