#! /usr/bin/sh
mkdir -p build
cd build
make
./tests/YunGameServer_test
cd ..