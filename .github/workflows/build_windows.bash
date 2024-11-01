#! /usr/bin/env bash

export CMAKE_INSTALL_PREFIX="C:/ProgramData/libgit2"
(
    git clone https://github.com/libgit2/libgit2.git
    mkdir libgit2/build
    cd libgit2/build
    cmake .. -DCMAKE_C_COMPILER=gcc -DBUILD_CLI=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
    cmake --build . --target install --parallel 4
)

export PKG_CONFIG_PATH="C:/ProgramData/libgit2/lib/pkgconfig"
LDFLAGS="-static"
LDLIBS="-lcrypt32 -lole32 -lrpcrt4 -lstdc++ -lwinhttp -lws2_32 -lz $(pkg-config --libs libgit2)"
(
    cd custom-prompt
    make LDFLAGS="$LDFLAGS" LDLIBS="$LDLIBS" -j release
)
