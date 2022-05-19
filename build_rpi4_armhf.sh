#!/usr/bin/env sh

if [ ! -d "build" ]; then
  mkdir build
fi

if [ ! -d "lib" ]; then
  mkdir lib
fi

if [ ! -d "bin" ]; then
  mkdir bin
fi

cp "NDI SDK for Linux"/include/* include/
cp "NDI SDK for Linux"/lib/arm-rpi4-linux-gnueabihf/* lib/
cp "NDI SDK for Linux"/bin/arm-rpi4-linux-gnueabihf/ndi-record bin/

g++ -std=c++14 -pthread  -Wl,--allow-shlib-undefined -Wl,--as-needed -Iinclude/ -L lib -o build/ndi_recorder ndi_recorder.cpp mongoose.c mjson.c -lndi -ldl