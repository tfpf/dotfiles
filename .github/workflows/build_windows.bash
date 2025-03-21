#! /usr/bin/env bash

export CMAKE_INSTALL_PREFIX="C:/ProgramData/libgit2"
git clone https://github.com/libgit2/libgit2.git
mkdir libgit2/build
(
    cd libgit2/build
    cmake .. -DBUILD_CLI=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
    cmake --build . --target install --parallel 4
)

export PKG_CONFIG_PATH="$CMAKE_INSTALL_PREFIX/lib/pkgconfig"
cd custom-prompt
make LDFLAGS="-flto -O2 -static" LDLIBS="-lstdc++ -lwinhttp $(pkg-config --libs --static libgit2)" -j release
