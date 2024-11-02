#! /usr/bin/env bash

(
    git clone https://github.com/libgit2/libgit2.git
    mkdir libgit2/build
    cd libgit2/build
    cmake .. -DBUILD_CLI=OFF -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF
    cmake --build . --target install --parallel 4
)

# MSYS2 sets this environment variable, overriding what is specified in the
# workflow file. Hence, redefine it here.
export PKG_CONFIG_PATH="$CMAKE_INSTALL_PREFIX/lib/pkgconfig"
LDFLAGS="-flto -O2 -static"
LDLIBS="-lstdc++ -lwinhttp $(pkg-config --libs --static libgit2)"
(
    cd custom-prompt
    make LDFLAGS="$LDFLAGS" LDLIBS="$LDLIBS" -j release
)
