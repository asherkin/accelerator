#!/bin/sh
set -ex

cd third_party/breakpad
git reset --hard
git apply ../../patches/*.patch

cd src/third_party
ln -sf ../../../protobuf protobuf
ln -sf ../../../lss lss
cd ../..

mkdir -p build
cd build
mkdir -p x86
mkdir -p x86_64

cd x86
../../configure --enable-m32 CXXFLAGS="-g -O2 -D_GLIBCXX_USE_CXX11_ABI=0"

make src/tools/linux/dump_syms/dump_syms
make src/client/linux/libbreakpad_client.a
make src/libbreakpad.a src/third_party/libdisasm/libdisasm.a
cd ..

cd x86_64
../../configure CXXFLAGS="-g -O2 -D_GLIBCXX_USE_CXX11_ABI=0"

make src/tools/linux/dump_syms/dump_syms
make src/client/linux/libbreakpad_client.a
make src/libbreakpad.a src/third_party/libdisasm/libdisasm.a