#!/bin/bash
mkdir build-emscripten
cd build-emscripten

~/bin/emsdk/upstream/emscripten/emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
~/bin/emsdk/upstream/emscripten/emmake make
